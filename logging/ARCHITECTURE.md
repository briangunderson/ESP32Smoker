# ESP32 Logging Infrastructure - Architecture

## System Architecture

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         ESP32 IoT Devices Layer                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌─────────────┐ │
│  │ ESP32Smoker  │  │ ESP32Smoker2 │  │ ESP32Weather │  │ ESP32...    │ │
│  │ 192.168.1.10 │  │ 192.168.1.11 │  │ 192.168.1.12 │  │ Future...   │ │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬──────┘ │
│         │                 │                 │                 │          │
│         └─────────────────┴─────────────────┴─────────────────┘          │
│                                    │                                     │
│                        UDP Syslog (RFC 5424)                             │
│                        Port 9514                                         │
└─────────────────────────────────────┬───────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                     Cribl Stream - Data Collection Layer                 │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │  Syslog Source (UDP 9514)                                         │  │
│  │  • Validates RFC 5424 format                                      │  │
│  │  • Parses priority, timestamp, hostname, app, message             │  │
│  └─────────────────────────────┬─────────────────────────────────────┘  │
│                                │                                         │
│  ┌─────────────────────────────▼─────────────────────────────────────┐  │
│  │  Processing Pipeline                                              │  │
│  │  • Parse priority → facility + severity                           │  │
│  │  • Extract structured data (temp, PID, state, relays)             │  │
│  │  • Add tags: ['esp32', 'iot', 'syslog']                          │  │
│  │  • Classify log type                                              │  │
│  └─────────────────────────────┬─────────────────────────────────────┘  │
│                                │                                         │
│  ┌─────────────────────────────▼─────────────────────────────────────┐  │
│  │  Route Logic                                                      │  │
│  │  • IF appname='smoker' OR host='ESP32*' → Elasticsearch          │  │
│  │  • ELSE → devnull                                                 │  │
│  └─────────────────────────────┬─────────────────────────────────────┘  │
│                                │                                         │
└────────────────────────────────┼─────────────────────────────────────────┘
                                 │ HTTP POST (Bulk API)
                                 │ Compressed (gzip)
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                Elasticsearch - Storage & Indexing Layer                  │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │  Ingest Pipeline: esp32-syslog-pipeline                           │  │
│  │  ┌─────────────────────────────────────────────────────────────┐ │  │
│  │  │ 1. Parse syslog with grok patterns                          │ │  │
│  │  │ 2. Calculate facility & severity from priority              │ │  │
│  │  │ 3. Map severity numbers to names (info, error, etc.)        │ │  │
│  │  │ 4. Extract temperature: /Current:\s*(\d+\.\d+)°F/           │ │  │
│  │  │ 5. Extract setpoint: /Setpoint:\s*(\d+\.\d+)°F/             │ │  │
│  │  │ 6. Extract PID: /P=(.+), I=(.+), D=(.+), Output=(.+)/       │ │  │
│  │  │ 7. Extract state: /State.*->\s*(\w+)/                       │ │  │
│  │  │ 8. Extract relays: /Auger:\s*(\w+).*Fan:\s*(\w+).../        │ │  │
│  │  │ 9. Convert types (string→float, string→boolean)             │ │  │
│  │  │ 10. Add log_type classification                             │ │  │
│  │  │ 11. Add tags array                                          │ │  │
│  │  │ 12. Set @timestamp                                          │ │  │
│  │  └─────────────────────────────────────────────────────────────┘ │  │
│  └─────────────────────────────┬─────────────────────────────────────┘  │
│                                │                                         │
│  ┌─────────────────────────────▼─────────────────────────────────────┐  │
│  │  Index Template: esp32-logs                                       │  │
│  │  • Pattern: esp32-logs-*                                          │  │
│  │  • Default pipeline: esp32-syslog-pipeline                        │  │
│  │  • 30+ field mappings                                             │  │
│  │  • Refresh interval: 5 seconds                                    │  │
│  └─────────────────────────────┬─────────────────────────────────────┘  │
│                                │                                         │
│  ┌─────────────────────────────▼─────────────────────────────────────┐  │
│  │  Time-Based Indices                                               │  │
│  │  • esp32-logs-2026.01.30  ← Today                                 │  │
│  │  • esp32-logs-2026.01.31  ← Tomorrow (auto-created)               │  │
│  │  • esp32-logs-2026.02.01  ← Future                                │  │
│  │  • ...                                                            │  │
│  │  Each index: 1 shard, 1 replica                                  │  │
│  └───────────────────────────────────────────────────────────────────┘  │
│                                                                          │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │  Watchers (Alerting)                          Check Interval      │  │
│  │  ┌─────────────────────────────────────────┬──────────────────┐   │  │
│  │  │ 1. Error State Alert                    │ Every 1 minute   │   │  │
│  │  │    Query: state = "ERROR"                │                  │   │  │
│  │  │    Action: Log + Index to esp32-alerts   │                  │   │  │
│  │  ├─────────────────────────────────────────┼──────────────────┤   │  │
│  │  │ 2. Temperature Out of Range             │ Every 1 minute   │   │  │
│  │  │    Query: temp < 50 OR temp > 500        │                  │   │  │
│  │  │    Action: Log + Index to esp32-alerts   │                  │   │  │
│  │  ├─────────────────────────────────────────┼──────────────────┤   │  │
│  │  │ 3. Consecutive Sensor Failures          │ Every 1 minute   │   │  │
│  │  │    Query: sensor errors >= 3 in 5min     │                  │   │  │
│  │  │    Action: Log + Index to esp32-alerts   │                  │   │  │
│  │  ├─────────────────────────────────────────┼──────────────────┤   │  │
│  │  │ 4. High Error Rate                      │ Every 5 minutes  │   │  │
│  │  │    Query: errors >= 10 in 15min          │                  │   │  │
│  │  │    Action: Log + Index to esp32-alerts   │                  │   │  │
│  │  └─────────────────────────────────────────┴──────────────────┘   │  │
│  └───────────────────────────────────────────────────────────────────┘  │
│                                                                          │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │  Alert Storage                                                    │  │
│  │  • esp32-alerts index                                             │  │
│  │  • Stores all triggered alerts                                    │  │
│  │  • Fields: alert_type, timestamp, device, severity, details       │  │
│  └───────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────┬─────────────────────────────────────────┘
                                 │ HTTP API (Port 9200)
                                 │
                    ┌────────────┴────────────┐
                    │                         │
                    ▼                         ▼
┌──────────────────────────────┐   ┌────────────────────────────┐
│  Kibana - Visualization      │   │  Direct API Access         │
│                              │   │                            │
│  ┌────────────────────────┐ │   │  curl commands             │
│  │ Index Pattern          │ │   │  Scripts                   │
│  │ esp32-logs-*           │ │   │  External integrations     │
│  │ Time: @timestamp       │ │   │                            │
│  └────────────────────────┘ │   └────────────────────────────┘
│                              │
│  ┌────────────────────────┐ │
│  │ 8 Visualizations       │ │
│  │                        │ │
│  │ 1. Log Level Pie       │ │
│  │ 2. Logs Timeline       │ │
│  │ 3. Error Rate Metric   │ │
│  │ 4. Current State       │ │
│  │ 5. Temp Timeline       │ │
│  │ 6. PID Performance     │ │
│  │ 7. Recent Errors Table │ │
│  │ 8. State Transitions   │ │
│  └────────────────────────┘ │
│                              │
│  ┌────────────────────────┐ │
│  │ Dashboard              │ │
│  │ "ESP32 Smoker Monitor" │ │
│  │                        │ │
│  │ Auto-refresh: 30s      │ │
│  │ Time range: 1 hour     │ │
│  │ Filters: device, sev   │ │
│  └────────────────────────┘ │
│                              │
│  ┌────────────────────────┐ │
│  │ Discover (Ad-hoc)      │ │
│  │ Search all logs        │ │
│  │ Filter, aggregate      │ │
│  └────────────────────────┘ │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│   Browser (User Interface)   │
│   http://YOUR-LOGGING-SERVER-IP:5601   │
└──────────────────────────────┘
```

## Data Flow - Detailed

### Step-by-Step Log Processing

```
1. ESP32 DEVICE
   │
   ├─ Creates log message
   │  Example: "Temperature: 225.5°F, Setpoint: 225.0°F"
   │
   ├─ Formats as RFC 5424 syslog
   │  <134>1 2026-01-30T12:34:56.789Z ESP32Smoker smoker 1234 - - [Temperature] Current: 225.5°F, Setpoint: 225.0°F
   │  │    │                      │             │      │    │  │                                 │
   │  │    │                      │             │      │    │  │                                 └─ Message
   │  │    │                      │             │      │    │  └─ Structured Data (unused)
   │  │    │                      │             │      │    └─ Message ID (unused)
   │  │    │                      │             │      └─ Process ID
   │  │    │                      │             └─ App Name
   │  │    │                      └─ Hostname
   │  │    └─ Timestamp (ISO 8601)
   │  └─ Priority (facility=16, severity=6)
   │
   └─ Sends via UDP to YOUR-LOGGING-SERVER-IP:9514

2. CRIBL STREAM
   │
   ├─ Receives on UDP port 9514
   │
   ├─ Validates RFC 5424 format
   │  ✓ Valid → continue
   │  ✗ Invalid → discard
   │
   ├─ Checks routing filter
   │  IF appname == 'smoker' OR hostname includes 'ESP32'
   │    → Route to Elasticsearch
   │  ELSE
   │    → Route to devnull
   │
   ├─ Applies pipeline functions
   │  • Parse priority
   │  • Add severity name
   │  • Extract temperature
   │  • Add tags
   │
   └─ Sends to Elasticsearch via HTTP POST
      Endpoint: http://YOUR-LOGGING-SERVER-IP:9200/_bulk
      Auth: Basic (elastic:YOUR-ELASTIC-PASSWORD)
      Format: NDJSON (newline-delimited JSON)
      Compression: gzip

3. ELASTICSEARCH INGEST PIPELINE
   │
   ├─ Receives document:
   │  {
   │    "message": "<134>1 2026-01-30T12:34:56.789Z ESP32Smoker smoker 1234 - - [Temperature] Current: 225.5°F, Setpoint: 225.0°F",
   │    "host": "192.168.1.10",
   │    ...
   │  }
   │
   ├─ Processor 1: Grok - Parse syslog format
   │  Extracts: priority, timestamp, device_name, app_name, message
   │
   ├─ Processor 2: Script - Calculate facility & severity
   │  facility = priority / 8 = 134 / 8 = 16
   │  severity_num = priority % 8 = 134 % 8 = 6
   │
   ├─ Processor 3: Script - Map severity to name
   │  severity_num = 6 → severity = "info"
   │
   ├─ Processor 4: Date - Parse timestamp
   │  syslog_timestamp → @timestamp
   │
   ├─ Processor 5: Grok - Extract temperature
   │  Pattern: /Current:\s*(\d+\.\d+)°F/
   │  Result: temperature = 225.5
   │
   ├─ Processor 6: Grok - Extract setpoint
   │  Pattern: /Setpoint:\s*(\d+\.\d+)°F/
   │  Result: setpoint = 225.0
   │
   ├─ Processor 7-10: Extract PID, state, relays (if present)
   │
   ├─ Processor 11: Script - Add log_type
   │  Has temperature? → log_type = "temperature"
   │
   ├─ Processor 12: Script - Add tags
   │  tags = ["esp32", "iot", "syslog", "smoker"]
   │
   └─ Output document:
      {
        "@timestamp": "2026-01-30T12:34:56.789Z",
        "device_name": "ESP32Smoker",
        "app_name": "smoker",
        "priority": 134,
        "facility": 16,
        "facility_name": "local0",
        "severity_num": 6,
        "severity": "info",
        "message": "[Temperature] Current: 225.5°F, Setpoint: 225.0°F",
        "temperature": 225.5,
        "setpoint": 225.0,
        "log_type": "temperature",
        "tags": ["esp32", "iot", "syslog", "smoker"],
        "host": "192.168.1.10"
      }

4. ELASTICSEARCH INDEX
   │
   ├─ Determines target index
   │  Template pattern: esp32-logs-*
   │  Current date: 2026.01.30
   │  → Index: esp32-logs-2026.01.30
   │
   ├─ Applies index template
   │  • Sets field mappings (temperature: float, etc.)
   │  • Sets number_of_shards: 1
   │  • Sets number_of_replicas: 1
   │
   ├─ Stores document
   │  Document ID: auto-generated
   │  Index: esp32-logs-2026.01.30
   │  Type: _doc
   │
   └─ Document is now searchable (after 5s refresh)

5. WATCHERS (Background Process)
   │
   ├─ Error State Watcher (runs every 1 minute)
   │  Query: state = "ERROR" AND @timestamp > now-2m
   │  IF hits > 0
   │    → Log error
   │    → Create alert document in esp32-alerts
   │
   ├─ Temperature Range Watcher (runs every 1 minute)
   │  Query: (temperature < 50 OR temperature > 500) AND @timestamp > now-2m
   │  IF hits > 0
   │    → Log warning
   │    → Create alert document in esp32-alerts
   │
   ├─ Sensor Failures Watcher (runs every 1 minute)
   │  Query: message matches sensor error patterns AND @timestamp > now-5m
   │  IF count >= 3
   │    → Log error
   │    → Create alert document in esp32-alerts
   │
   └─ High Error Rate Watcher (runs every 5 minutes)
      Query: severity IN [error, critical] AND @timestamp > now-15m
      IF count >= 10
        → Log warning
        → Create alert document in esp32-alerts

6. KIBANA DASHBOARD
   │
   ├─ User opens dashboard
   │  URL: http://YOUR-LOGGING-SERVER-IP:5601/app/dashboards#/view/esp32-dashboard
   │
   ├─ Dashboard loads 8 visualizations
   │  Each visualization queries Elasticsearch:
   │
   │  Visualization 1: Log Level Distribution
   │    Query: Aggregate by severity (last 1 hour)
   │    Result: {info: 1200, warning: 45, error: 5}
   │    Display: Pie chart
   │
   │  Visualization 5: Temperature Timeline
   │    Query: avg(temperature), avg(setpoint) grouped by 1-minute buckets
   │    Result: Time series data
   │    Display: Line chart (2 lines)
   │
   │  Visualization 3: Error Rate
   │    Query: count where severity IN [error, critical] AND time > now-15m
   │    Result: 5
   │    Display: Metric with color (green if < 5, red if >= 20)
   │
   │  ... and 5 more visualizations
   │
   ├─ Auto-refresh triggers every 30 seconds
   │  Re-runs all queries
   │  Updates all visualizations
   │
   └─ User can:
      • Click on visualizations to drill down
      • Adjust time range (last hour, last day, etc.)
      • Filter by device_name or severity
      • Switch to Discover for ad-hoc searches

7. USER INTERACTION
   │
   ├─ View real-time temperature
   │  → Temperature Timeline shows live updates
   │
   ├─ Investigate an error spike
   │  → Click on red area in Logs Over Time
   │  → Filters to that time range
   │  → Recent Errors table shows details
   │  → Click message to see full document
   │
   ├─ Search for specific device
   │  → Add filter: device_name = "ESP32Smoker2"
   │  → All visualizations update to show only that device
   │
   └─ Export data for analysis
      → Use Discover to search
      → Export as CSV or JSON
```

## Component Interactions

### ESP32 ↔ Cribl

**Protocol:** UDP Syslog (RFC 5424)
**Port:** 9514
**Format:** `<PRI>VER TIMESTAMP HOSTNAME APP-NAME PROCID MSGID STRUCTURED-DATA MSG`

**Reliability:**
- UDP = fire-and-forget (no acknowledgment)
- Cribl buffers in memory
- If Cribl is down, logs are lost (no retry)
- Consider logging to SD card as backup

**Security:**
- No encryption (plain text UDP)
- Consider VPN or isolated VLAN for production

### Cribl ↔ Elasticsearch

**Protocol:** HTTP/HTTPS
**Port:** 9200
**Auth:** Basic (username/password)
**Format:** NDJSON (Bulk API)

**Reliability:**
- HTTP with retries
- Cribl queues if ES is down
- Configurable batch size and timeout

**Performance:**
- Bulk operations (500 events per batch)
- gzip compression
- Configurable flush interval (1 second)

### Elasticsearch ↔ Kibana

**Protocol:** HTTP/HTTPS
**Port:** 9200 (ES API)
**Auth:** Basic (shared with Cribl)
**Format:** JSON (Search API)

**Reliability:**
- Kibana queries ES in real-time
- No caching (always live data)
- If ES is down, Kibana shows errors

**Performance:**
- Queries limited by time range
- Aggregations pre-computed
- Caching at browser level

## Scaling Considerations

### Current Configuration (Single Node)

```
ESP32 Devices: 1-10 devices
Log Rate: ~100 logs/minute per device = ~1,000/min total
Data Rate: ~100 KB/min
Storage: ~150 MB/day

Cribl: Single instance
  - Can handle 10,000+ events/sec
  - Memory: 512 MB adequate
  - CPU: 1 core sufficient

Elasticsearch: Single node
  - Heap: 2 GB
  - Disk: 50 GB (for ~1 year of logs)
  - Shards: 1 per index (daily)
  - Replicas: 1 (for backup)

Kibana: Single instance
  - Memory: 1 GB
  - CPU: 1 core
```

### High Volume (100+ Devices)

```
ESP32 Devices: 100 devices
Log Rate: ~10,000 logs/minute
Data Rate: ~10 MB/min
Storage: ~15 GB/day

Cribl: Clustered (3 nodes)
  - Load balanced
  - High availability
  - Memory: 2 GB per node

Elasticsearch: Cluster (3 nodes)
  - Heap: 4-8 GB per node
  - Disk: 500 GB (SSD) per node
  - Shards: 3 per index
  - Replicas: 1

Kibana: Multiple instances
  - Behind load balancer
  - Memory: 2 GB per instance
```

### Optimization Tips

**For Low Resources:**
1. Reduce replicas to 0 (single node)
2. Increase refresh interval to 30s
3. Delete old indices more frequently
4. Reduce shard count
5. Disable unused features

**For High Performance:**
1. Use SSD storage
2. Increase Elasticsearch heap (50% of RAM)
3. Add more shards for parallel processing
4. Use dedicated master nodes
5. Enable slow log monitoring
6. Tune bulk size and flush intervals

## Security Architecture

### Current Security Posture

```
┌─────────────────────────────────────────────────┐
│ AUTHENTICATION                                  │
├─────────────────────────────────────────────────┤
│ ESP32 → Cribl         None (UDP, no auth)      │
│ Cribl → Elasticsearch Basic Auth (user/pass)   │
│ User → Kibana         Basic Auth (user/pass)   │
│ User → Elasticsearch  Basic Auth (user/pass)   │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│ ENCRYPTION                                      │
├─────────────────────────────────────────────────┤
│ ESP32 → Cribl         None (plain text UDP)    │
│ Cribl → Elasticsearch HTTP (optional HTTPS)    │
│ User → Kibana         HTTP (optional HTTPS)    │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│ AUTHORIZATION                                   │
├─────────────────────────────────────────────────┤
│ Elasticsearch         Single user (elastic)     │
│                      Full admin privileges      │
│ Kibana               Same user, full access     │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│ NETWORK SECURITY                                │
├─────────────────────────────────────────────────┤
│ Firewall             Allow UDP 9514 (Cribl)     │
│                      Allow TCP 5601 (Kibana)    │
│                      Allow TCP 9200 (ES - opt)  │
│ Network Isolation    All on same subnet        │
└─────────────────────────────────────────────────┘
```

### Recommendations for Production

1. **Enable HTTPS** on all HTTP connections
2. **Create dedicated users** with least-privilege access
   - Read-only user for Kibana viewers
   - Write-only user for Cribl
   - Admin user for maintenance only
3. **Use TLS for syslog** (TCP with TLS instead of UDP)
4. **Implement network segmentation** (VLANs)
5. **Enable audit logging** in Elasticsearch
6. **Set up IP allowlists** on Cribl syslog listener
7. **Rotate passwords** regularly
8. **Use API keys** instead of passwords where possible

## Monitoring the Monitor

### Health Checks

```bash
# Check Cribl status
curl http://YOUR-LOGGING-SERVER-IP:9000/api/v1/health

# Check Elasticsearch cluster health
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:9200/_cluster/health?pretty

# Check Kibana status
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:5601/api/status

# Check log ingestion rate
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  "http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-*/_search?pretty" \
  -H 'Content-Type: application/json' \
  -d '{
    "size": 0,
    "aggs": {
      "logs_per_minute": {
        "date_histogram": {
          "field": "@timestamp",
          "fixed_interval": "1m"
        }
      }
    }
  }'
```

### Key Metrics to Monitor

1. **Cribl:**
   - Input rate (events/sec)
   - Output rate (events/sec)
   - Dropped events
   - Memory usage
   - CPU usage

2. **Elasticsearch:**
   - Cluster health (green/yellow/red)
   - JVM heap usage
   - Disk usage
   - Indexing rate
   - Search rate
   - Query latency

3. **Kibana:**
   - Response time
   - Active sessions
   - Memory usage

4. **Application:**
   - Log ingestion rate
   - Parse success rate
   - Alert trigger frequency
   - Data retention compliance

### Alerting on Infrastructure Issues

Create watchers for:
- Elasticsearch heap > 85%
- Disk usage > 90%
- Cluster health = red
- No logs received in 5 minutes
- Parse failure rate > 5%

## Disaster Recovery

### Backup Strategy

**What to Backup:**
1. Elasticsearch configuration
2. Index templates
3. Ingest pipelines
4. Watchers
5. Kibana saved objects (dashboards, visualizations)
6. Cribl configuration

**How to Backup:**
```bash
# Elasticsearch configs
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:9200/_index_template/esp32-logs \
  > backup/index-template.json

curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:9200/_ingest/pipeline/esp32-syslog-pipeline \
  > backup/ingest-pipeline.json

# Kibana objects
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:5601/api/saved_objects/_export \
  -H 'kbn-xsrf: true' \
  -H 'Content-Type: application/json' \
  -d '{"type": ["dashboard", "visualization", "index-pattern"]}' \
  > backup/kibana-objects.ndjson

# Data snapshot (for indices)
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X PUT "http://YOUR-LOGGING-SERVER-IP:9200/_snapshot/my_backup/snapshot_1?wait_for_completion=true"
```

**Backup Frequency:**
- Configurations: After each change
- Indices: Daily (automated snapshot)

### Recovery Procedures

**Configuration Loss:**
```bash
# Re-run setup scripts
cd logging/scripts
./setup-elasticsearch.sh
./setup-kibana.sh
./setup-watchers.sh
```

**Data Loss:**
```bash
# Restore from snapshot
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X POST "http://YOUR-LOGGING-SERVER-IP:9200/_snapshot/my_backup/snapshot_1/_restore"
```

**Complete System Failure:**
1. Reinstall Cribl, Elasticsearch, Kibana
2. Run all setup scripts
3. Restore data from snapshots
4. Verify with test-logging.sh

**Recovery Time Objective (RTO):** ~1 hour
**Recovery Point Objective (RPO):** 24 hours (daily backups)

## Future Enhancements

### Phase 2: Enhanced Analytics
- Machine learning anomaly detection
- Predictive maintenance alerts
- Historical trend analysis
- Capacity planning recommendations

### Phase 3: Multi-Tenancy
- Separate indices per device type
- Role-based access control
- Per-device dashboards
- Isolated alert streams

### Phase 4: Integration
- Home Assistant integration
- Email/SMS notifications
- Slack/Discord webhooks
- Grafana dashboards
- Prometheus metrics export

### Phase 5: Edge Processing
- ESP32 log buffering (SD card)
- Offline operation support
- Batch upload on reconnect
- Local log aggregation

---

*This architecture is designed for clarity, maintainability, and scalability.*
*All components are modular and can be replaced or upgraded independently.*
