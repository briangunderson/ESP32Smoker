# ESP32 IoT Logging Infrastructure

## Overview

This is a comprehensive, production-ready logging infrastructure for ESP32 IoT devices using the Cribl/Elasticsearch/Kibana stack. It's designed to be modular and reusable for any ESP32 project.

### Architecture

```
ESP32 Devices (Syslog RFC 5424)
    |
    v (UDP port 9514)
Cribl Stream (Data collection & routing)
    |
    v (HTTP REST API)
Elasticsearch (Data storage & indexing)
    |
    v (Ingest pipeline & parsing)
Kibana (Visualization & dashboards)
    |
    v (User interface)
Browser (Monitoring & analysis)
```

### Key Features

- **Syslog RFC 5424 Support**: Standard protocol for reliable logging
- **Intelligent Parsing**: Automatic extraction of temperature, PID values, state transitions, and relay status
- **Real-time Monitoring**: 30-second auto-refresh dashboard
- **Advanced Alerting**: Critical event detection with Elasticsearch Watchers
- **Multi-device Support**: Handles logs from multiple ESP32 devices
- **Time-based Indexing**: Daily indices for efficient data management
- **Reusable Design**: Easy to adapt for other ESP32 projects

## Quick Start

### Prerequisites

- Cribl Stream running on YOUR-LOGGING-SERVER-IP:9000
- Elasticsearch running on YOUR-LOGGING-SERVER-IP:9200
- Kibana running on YOUR-LOGGING-SERVER-IP:5601
- Bash shell (Linux/Mac/WSL) for running setup scripts
- curl and nc (netcat) utilities

### Setup Steps

1. **Configure Elasticsearch** (5 minutes)
   ```bash
   cd logging/scripts
   ./setup-elasticsearch.sh
   ```
   This creates:
   - Index template for `esp32-logs-*`
   - Ingest pipeline for parsing syslog messages
   - Initial index

2. **Configure Kibana** (5 minutes)
   ```bash
   ./setup-kibana.sh
   ```
   This creates:
   - Index pattern `esp32-logs-*`
   - 8 visualizations (charts, metrics, tables)
   - Dashboard "ESP32 Smoker Monitoring"

3. **Configure Alerts** (2 minutes)
   ```bash
   ./setup-watchers.sh
   ```
   This creates:
   - 4 watchers for critical events
   - `esp32-alerts` index for alert storage

4. **Configure Cribl** (10 minutes)
   - Follow the manual steps in `logging/cribl/README.md`
   - Or import the configuration from `logging/cribl/pipeline-config.yml`

5. **Test the Pipeline** (2 minutes)
   ```bash
   ./test-logging.sh
   ```
   This sends sample log messages to verify everything works

6. **Configure Your ESP32**
   - Ensure your ESP32 sends syslog to `YOUR-LOGGING-SERVER-IP:9514` (UDP)
   - Use RFC 5424 format
   - Set device name (e.g., "ESP32Smoker")
   - Set app name (e.g., "smoker")

## Directory Structure

```
logging/
├── elasticsearch/          # Elasticsearch configurations
│   ├── index-template.json              # Index template
│   ├── ingest-pipeline.json             # Log parsing pipeline
│   ├── watcher-error-state.json         # Alert: ERROR state
│   ├── watcher-temperature-out-of-range.json  # Alert: Temp limits
│   ├── watcher-consecutive-sensor-failures.json  # Alert: Sensor errors
│   └── watcher-high-error-rate.json     # Alert: High error rate
│
├── kibana/                 # Kibana configurations
│   ├── index-pattern.json               # Index pattern definition
│   ├── dashboard.json                   # Main dashboard
│   ├── viz-log-level-distribution.json  # Pie chart: log levels
│   ├── viz-logs-over-time.json          # Area chart: logs timeline
│   ├── viz-error-rate.json              # Metric: error count
│   ├── viz-device-status.json           # Metric: current state
│   ├── viz-temperature-timeline.json    # Line chart: temperature
│   ├── viz-pid-performance.json         # Line chart: PID values
│   ├── viz-recent-errors.json           # Table: recent errors
│   └── viz-state-transitions.json       # Table: state changes
│
├── cribl/                  # Cribl configurations
│   ├── pipeline-config.yml              # Complete pipeline config
│   └── README.md                        # Setup instructions
│
├── scripts/                # Setup & test scripts
│   ├── setup-elasticsearch.sh           # Configure Elasticsearch
│   ├── setup-kibana.sh                  # Configure Kibana
│   ├── setup-watchers.sh                # Configure alerts
│   └── test-logging.sh                  # Send test messages
│
└── README.md               # This file
```

## Data Flow

### 1. Log Generation (ESP32)

ESP32 devices send syslog messages in RFC 5424 format:

```
<134>1 2026-01-30T12:34:56.789Z ESP32Smoker smoker 1234 - - [Temperature] Current: 225.5°F, Setpoint: 225.0°F
```

Components:
- `<134>`: Priority (facility=16, severity=6/info)
- `1`: Syslog version
- `2026-01-30T12:34:56.789Z`: Timestamp (ISO 8601)
- `ESP32Smoker`: Device hostname
- `smoker`: Application name
- `1234`: Process ID
- `-`: Message ID (not used)
- `-`: Structured data (not used)
- `[Temperature] Current: ...`: The actual log message

### 2. Data Collection (Cribl)

Cribl receives UDP syslog on port 9514 and:
- Validates RFC 5424 format
- Adds tags: `['esp32', 'iot', 'syslog']`
- Routes ESP32 logs to Elasticsearch
- Discards non-ESP32 traffic

### 3. Data Processing (Elasticsearch Ingest Pipeline)

The ingest pipeline:
- Parses priority into facility and severity
- Extracts structured data using regex:
  - Temperature values
  - Setpoint values
  - PID control parameters
  - State transitions
  - Relay status
  - Error codes
- Adds log_type classification
- Converts data types (strings to floats/booleans)
- Sets @timestamp field

### 4. Data Storage (Elasticsearch)

Logs are stored in time-based indices:
- `esp32-logs-2026.01.30` (today)
- `esp32-logs-2026.01.31` (tomorrow)
- Automatic creation based on index template
- 1 shard, 1 replica (configurable)
- Refresh interval: 5 seconds

### 5. Visualization (Kibana)

Dashboard shows:
- Real-time temperature vs setpoint
- PID controller performance
- State machine transitions
- Error rate and recent errors
- Log level distribution
- Current system state

### 6. Alerting (Elasticsearch Watchers)

Watchers monitor for:
- Device enters ERROR state
- Temperature < 50°F or > 500°F
- 3+ sensor failures in 5 minutes
- 10+ errors in 15 minutes

Alerts are:
- Logged to Elasticsearch logs
- Indexed to `esp32-alerts` index
- Can be extended to send email/Slack notifications

## Field Mapping

### Standard Fields

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| @timestamp | date | Log timestamp | 2026-01-30T12:34:56.789Z |
| device_name | keyword | Device hostname | ESP32Smoker |
| app_name | keyword | Application name | smoker |
| priority | integer | Syslog priority | 134 |
| severity | keyword | Severity name | info |
| severity_num | integer | Severity number (0-7) | 6 |
| facility | integer | Facility number | 16 |
| facility_name | keyword | Facility name | local0 |
| message | text | Log message | [Temperature] Current: 225.5°F |
| host | keyword | Source IP address | 192.168.1.100 |
| pid | integer | Process ID | 1234 |
| tags | keyword[] | Tags | ['esp32', 'iot', 'smoker'] |
| log_type | keyword | Log classification | temperature |

### Extracted Fields

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| temperature | float | Current temperature (°F) | 225.5 |
| setpoint | float | Target temperature (°F) | 225.0 |
| pid_p | float | PID proportional term | 2.5 |
| pid_i | float | PID integral term | 0.8 |
| pid_d | float | PID derivative term | 0.1 |
| pid_output | float | PID output value | 45.2 |
| state | keyword | System state | RUNNING |
| relay_auger | boolean | Auger relay status | true |
| relay_fan | boolean | Fan relay status | true |
| relay_igniter | boolean | Igniter relay status | false |
| error_code | keyword | Error identifier | SENSOR_FAULT |
| error_count | integer | Error occurrence count | 3 |

## Log Message Patterns

The ingest pipeline recognizes these patterns:

### Temperature Logs
```
[Temperature] Current: 225.5°F, Setpoint: 225.0°F
Temperature: 225.5°F
Setpoint: 225.0°F
```
Extracts: `temperature`, `setpoint`

### PID Control Logs
```
[PID] P=2.5, I=0.8, D=0.1, Output=45.2
PID: P=2.5 I=0.8 D=0.1 Output=45.2
```
Extracts: `pid_p`, `pid_i`, `pid_d`, `pid_output`

### State Transition Logs
```
State transition: STARTUP -> RUNNING
Entering state: RUNNING
State: RUNNING
```
Extracts: `state`

### Relay Control Logs
```
[Relay] Auger: on, Fan: on, Igniter: off
Auger: on Fan: on Igniter: off
```
Extracts: `relay_auger`, `relay_fan`, `relay_igniter`

### Error Logs
```
ERROR: SENSOR_FAULT
Error code: MAX31865_FAULT
```
Extracts: `error_code`

## Kibana Dashboard

### Dashboard: "ESP32 Smoker Monitoring"

**Top Row (Metrics)**
- Error Rate (15min): Red/yellow/green status indicator
- Current State: Latest system state from logs
- Log Level Distribution: Pie chart of severity levels
- Logs Over Time: Area chart with severity breakdown

**Middle Row (Performance)**
- Temperature Timeline: Current temp vs setpoint over time
- PID Performance: P, I, D, and Output values over time

**Bottom Row (Diagnostics)**
- Recent Errors: Table of last 50 error/critical messages
- State Transitions: Timeline of state changes

**Dashboard Features**
- Auto-refresh: 30 seconds
- Time range: Last 1 hour (adjustable)
- Filters: device_name, severity
- Direct links to Discover for detailed analysis

### Accessing the Dashboard

1. Open Kibana: http://YOUR-LOGGING-SERVER-IP:5601
2. Navigate to **Dashboards**
3. Search for "ESP32 Smoker Monitoring"
4. Click to open

Or direct URL:
```
http://YOUR-LOGGING-SERVER-IP:5601/app/dashboards#/view/esp32-dashboard
```

## Alerts & Monitoring

### Configured Alerts

#### 1. Error State Alert
- **Trigger**: Device enters ERROR state
- **Check interval**: 1 minute
- **Condition**: Any log with `state = "ERROR"` in last 2 minutes
- **Actions**: Log error, index to esp32-alerts
- **Severity**: Critical

#### 2. Temperature Out of Range Alert
- **Trigger**: Temperature < 50°F or > 500°F
- **Check interval**: 1 minute
- **Condition**: Temperature outside safe range
- **Actions**: Log warning, index to esp32-alerts
- **Severity**: Critical

#### 3. Consecutive Sensor Failures Alert
- **Trigger**: 3+ sensor read errors in 5 minutes
- **Check interval**: 1 minute
- **Condition**: 3 or more logs matching sensor error patterns
- **Actions**: Log error, index to esp32-alerts
- **Severity**: Critical

#### 4. High Error Rate Alert
- **Trigger**: 10+ error/critical logs in 15 minutes
- **Check interval**: 5 minutes
- **Condition**: Error count exceeds threshold
- **Actions**: Log warning, index to esp32-alerts
- **Severity**: Warning

### Viewing Alerts

**In Kibana:**
1. Create index pattern for `esp32-alerts`
2. Go to **Discover**
3. Select `esp32-alerts` index
4. View all triggered alerts

**Query alerts:**
```bash
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:9200/esp32-alerts/_search?pretty
```

### Extending Alerts

To add email notifications:

1. Configure email settings in Elasticsearch:
   ```bash
   # In elasticsearch.yml
   xpack.notification.email.account:
     gmail_account:
       profile: gmail
       smtp:
         auth: true
         starttls.enable: true
         host: smtp.gmail.com
         port: 587
         user: your-email@gmail.com
       from: your-email@gmail.com
   ```

2. Add email action to watcher JSON:
   ```json
   "actions": {
     "send_email": {
       "email": {
         "to": "admin@example.com",
         "subject": "ESP32 Alert: {{ctx.payload.hits.hits.0._source.alert_type}}",
         "body": "Alert: {{ctx.payload.hits.hits.0._source.message}}"
       }
     }
   }
   ```

3. Re-run setup-watchers.sh

## Reusing for Other Projects

This infrastructure is designed to be reusable. Here's how to adapt it:

### 1. Minimal Changes (Same Message Format)

If your new ESP32 project uses similar log formats:

1. Update device_name and app_name in your ESP32 code
2. Logs automatically appear in the same indices
3. Use dashboard filters to view specific devices
4. No configuration changes needed

### 2. Different Message Formats

If your project has different log patterns:

1. **Update Ingest Pipeline** (`elasticsearch/ingest-pipeline.json`):
   - Add new grok patterns for your message formats
   - Add new fields to extract
   - Update field mappings

2. **Update Index Template** (`elasticsearch/index-template.json`):
   - Add mappings for new fields
   - Adjust field types as needed

3. **Create New Visualizations**:
   - Use existing visualizations as templates
   - Modify queries and aggregations
   - Update field references

4. **Update Dashboard**:
   - Add/remove visualizations
   - Adjust layout
   - Update filters

### 3. Completely Different Project

For a completely different ESP32 project:

1. **Copy the logging directory** to your new project

2. **Rename indices**:
   - Change `esp32-logs-*` to `yourproject-logs-*`
   - Update all JSON files with new index name
   - Search and replace in all files

3. **Update field mappings**:
   - Remove ESP32 Smoker-specific fields (temperature, pid_*, relay_*)
   - Add your project-specific fields
   - Update ingest pipeline grok patterns

4. **Recreate visualizations**:
   - Use the provided visualizations as examples
   - Build visualizations for your project's metrics
   - Create a custom dashboard

5. **Adjust alerts**:
   - Remove smoker-specific watchers
   - Create watchers for your project's critical events
   - Update alert thresholds

### Example: Adapting for a Weather Station

```bash
# 1. Copy the infrastructure
cp -r ESP32Smoker/logging ESP32Weather/logging

# 2. Update index names
cd ESP32Weather/logging
find . -type f -name "*.json" -exec sed -i 's/esp32-logs/esp32-weather/g' {} +

# 3. Edit ingest pipeline for weather data
# Add patterns like:
#   Temperature: (\d+\.\d+)°C
#   Humidity: (\d+)%
#   Pressure: (\d+) hPa

# 4. Update index template with weather fields:
#   temperature_c (float)
#   humidity (integer)
#   pressure (integer)
#   wind_speed (float)

# 5. Create visualizations:
#   - Temperature/humidity timeline
#   - Pressure gauge
#   - Wind speed chart

# 6. Run setup scripts
cd scripts
./setup-elasticsearch.sh
./setup-kibana.sh
```

## Troubleshooting

### No Data Appearing in Elasticsearch

**Check Cribl:**
1. Go to Cribl Live Data: http://YOUR-LOGGING-SERVER-IP:9000/monitoring/live-data
2. Filter by source: `syslog_udp`
3. Verify messages are being received

**Check ESP32:**
1. Verify ESP32 is sending to correct IP: YOUR-LOGGING-SERVER-IP
2. Verify port: 9514 (UDP)
3. Check firewall allows UDP 9514
4. Test with: `echo "test" | nc -u YOUR-LOGGING-SERVER-IP 9514`

**Check Elasticsearch:**
```bash
# Check if indices exist
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:9200/_cat/indices/esp32-logs-*?v

# Check if pipeline exists
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:9200/_ingest/pipeline/esp32-syslog-pipeline
```

### Parsing Errors

**Check ingest pipeline failures:**
```bash
# Search for documents with parse errors
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  "http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-*/_search?pretty" \
  -H 'Content-Type: application/json' \
  -d '{"query": {"exists": {"field": "error.message"}}}'
```

**Test ingest pipeline:**
```bash
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X POST "http://YOUR-LOGGING-SERVER-IP:9200/_ingest/pipeline/esp32-syslog-pipeline/_simulate?pretty" \
  -H 'Content-Type: application/json' \
  -d '{
    "docs": [{
      "source": {
        "message": "<134>1 2026-01-30T12:00:00Z ESP32Smoker smoker 1234 - - [Temperature] Current: 225.5°F, Setpoint: 225.0°F"
      }
    }]
  }'
```

### Visualizations Not Showing Data

1. **Check time range**: Dashboard defaults to last 1 hour
2. **Refresh index pattern**:
   - Go to Stack Management > Index Patterns
   - Select `esp32-logs-*`
   - Click refresh button (🔄)
3. **Check field filters**: Ensure no restrictive filters applied
4. **Verify data exists**:
   ```bash
   curl -u elastic:YOUR-ELASTIC-PASSWORD \
     "http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-*/_count?pretty"
   ```

### Alerts Not Triggering

1. **Check watcher status**:
   ```bash
   curl -u elastic:YOUR-ELASTIC-PASSWORD \
     "http://YOUR-LOGGING-SERVER-IP:9200/_watcher/stats?pretty"
   ```

2. **Manually execute watcher**:
   ```bash
   curl -u elastic:YOUR-ELASTIC-PASSWORD \
     -X POST "http://YOUR-LOGGING-SERVER-IP:9200/_watcher/watch/esp32-error-state/_execute?pretty"
   ```

3. **Check watcher history**:
   ```bash
   curl -u elastic:YOUR-ELASTIC-PASSWORD \
     "http://YOUR-LOGGING-SERVER-IP:9200/.watcher-history-*/_search?pretty"
   ```

### High Disk Usage

Elasticsearch stores data indefinitely by default. To manage disk space:

1. **Delete old indices**:
   ```bash
   # Delete indices older than 30 days
   curl -u elastic:YOUR-ELASTIC-PASSWORD \
     -X DELETE "http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-2026.01.01"
   ```

2. **Create Index Lifecycle Policy**:
   - Automatically delete indices after N days
   - Move old data to cheaper storage
   - Reduce replica count for old data

3. **Enable index rollover**:
   - Roll to new index after size/age limit
   - Automatically clean up old indices

## Performance Optimization

### For High Volume Logging

If you have multiple devices or high log rates:

1. **Increase bulk size** (Cribl config):
   ```yaml
   maxPayloadSizeKB: 8192
   maxPayloadEvents: 1000
   ```

2. **Adjust refresh interval** (Index template):
   ```json
   "index.refresh_interval": "10s"
   ```

3. **Increase shard count** (for > 50GB per day):
   ```json
   "number_of_shards": 3
   ```

4. **Use SSD storage** for Elasticsearch data directory

5. **Increase Elasticsearch heap size** (50% of available RAM):
   ```bash
   # In jvm.options
   -Xms4g
   -Xmx4g
   ```

### For Low Resource Systems

If running on limited hardware:

1. **Reduce refresh interval**:
   ```json
   "index.refresh_interval": "30s"
   ```

2. **Disable replicas** (single node):
   ```json
   "number_of_replicas": 0
   ```

3. **Reduce field data**:
   ```json
   "index.max_result_window": 5000
   ```

4. **Limit stored fields** in index template

## Maintenance

### Daily Tasks

- Check dashboard for anomalies
- Review alerts in `esp32-alerts` index
- Verify all devices are logging

### Weekly Tasks

- Review error patterns
- Check disk space usage
- Verify alert thresholds are appropriate

### Monthly Tasks

- Delete old indices (> 30 days)
- Review and optimize slow queries
- Update visualizations based on new insights
- Backup important indices

### Backup & Restore

**Backup index template:**
```bash
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  "http://YOUR-LOGGING-SERVER-IP:9200/_index_template/esp32-logs" \
  > backup-index-template.json
```

**Backup ingest pipeline:**
```bash
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  "http://YOUR-LOGGING-SERVER-IP:9200/_ingest/pipeline/esp32-syslog-pipeline" \
  > backup-ingest-pipeline.json
```

**Restore:**
```bash
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X PUT "http://YOUR-LOGGING-SERVER-IP:9200/_index_template/esp32-logs" \
  -H 'Content-Type: application/json' \
  -d @backup-index-template.json
```

## Configuration Reference

### Environment Variables

All setup scripts support these environment variables:

```bash
# Elasticsearch
export ES_HOST=YOUR-LOGGING-SERVER-IP
export ES_PORT=9200
export ES_USER=elastic
export ES_PASS=YOUR-ELASTIC-PASSWORD

# Kibana
export KIBANA_HOST=YOUR-LOGGING-SERVER-IP
export KIBANA_PORT=5601

# Cribl
export CRIBL_HOST=YOUR-LOGGING-SERVER-IP
export CRIBL_PORT=9514

# Device
export DEVICE_NAME=ESP32Smoker
export APP_NAME=smoker
```

### Syslog Priority Calculation

Priority = Facility × 8 + Severity

**Facilities:**
- 16 = local0 (recommended for ESP32)
- 17 = local1
- 18-23 = local2-7

**Severities:**
- 0 = emergency
- 1 = alert
- 2 = critical
- 3 = error
- 4 = warning
- 5 = notice
- 6 = info (default)
- 7 = debug

**Common priorities:**
- 134 = local0 + info
- 131 = local0 + error
- 130 = local0 + critical
- 135 = local0 + debug

## Additional Resources

### Documentation
- [Elasticsearch Documentation](https://www.elastic.co/guide/en/elasticsearch/reference/current/index.html)
- [Kibana Documentation](https://www.elastic.co/guide/en/kibana/current/index.html)
- [Cribl Documentation](https://docs.cribl.io/)
- [RFC 5424 Syslog Protocol](https://tools.ietf.org/html/rfc5424)

### Example Queries

**Search for errors in last hour:**
```json
GET /esp32-logs-*/_search
{
  "query": {
    "bool": {
      "must": [
        {"terms": {"severity": ["error", "critical"]}},
        {"range": {"@timestamp": {"gte": "now-1h"}}}
      ]
    }
  }
}
```

**Get average temperature per device:**
```json
GET /esp32-logs-*/_search
{
  "size": 0,
  "query": {"exists": {"field": "temperature"}},
  "aggs": {
    "by_device": {
      "terms": {"field": "device_name"},
      "aggs": {
        "avg_temp": {"avg": {"field": "temperature"}}
      }
    }
  }
}
```

**Find state transitions:**
```json
GET /esp32-logs-*/_search
{
  "query": {"exists": {"field": "state"}},
  "sort": [{"@timestamp": "desc"}],
  "size": 50
}
```

## Support

For issues or questions:

1. Check this README troubleshooting section
2. Review Cribl/Elasticsearch/Kibana logs
3. Test with `test-logging.sh` script
4. Verify configuration with setup scripts
5. Check individual component documentation

## License

This logging infrastructure is part of the ESP32 Smoker project and follows the same license.

## Version History

- **v1.0.0** (2026-01-30): Initial release
  - Complete Cribl/Elasticsearch/Kibana stack
  - 8 visualizations
  - 4 alert watchers
  - Automated setup scripts
  - Comprehensive documentation
