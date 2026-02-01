# ESP32 Logging Infrastructure - Deployment Checklist

## Pre-Deployment Verification

### Prerequisites
- [ ] Cribl Stream installed and running on YOUR-LOGGING-SERVER-IP:9000
- [ ] Elasticsearch installed and running on YOUR-LOGGING-SERVER-IP:9200
- [ ] Kibana installed and running on YOUR-LOGGING-SERVER-IP:5601
- [ ] Network connectivity between ESP32 and Cribl host
- [ ] Firewall allows UDP 9514 (ESP32 → Cribl)
- [ ] Bash shell available (Linux/Mac/WSL)
- [ ] curl utility installed
- [ ] nc (netcat) utility installed

### Access Credentials
- [ ] Cribl username: claude
- [ ] Cribl password: YOUR-CRIBL-PASSWORD
- [ ] Elasticsearch username: elastic
- [ ] Elasticsearch password: YOUR-ELASTIC-PASSWORD
- [ ] Kibana credentials: Same as Elasticsearch

### Configuration Review
- [ ] All JSON files are valid (no syntax errors)
- [ ] All shell scripts are executable
- [ ] IP addresses match your environment
- [ ] Port numbers are correct

## Deployment Steps

### Step 1: Elasticsearch Configuration (5 minutes)

```bash
cd logging/scripts
./setup-elasticsearch.sh
```

**Verify:**
- [ ] Script completes without errors
- [ ] Index template created
- [ ] Ingest pipeline created
- [ ] Initial index created

**Validation commands:**
```bash
# Check index template
curl -u elastic:YOUR-ELASTIC-PASSWORD http://YOUR-LOGGING-SERVER-IP:9200/_index_template/esp32-logs

# Check ingest pipeline
curl -u elastic:YOUR-ELASTIC-PASSWORD http://YOUR-LOGGING-SERVER-IP:9200/_ingest/pipeline/esp32-syslog-pipeline

# List indices
curl -u elastic:YOUR-ELASTIC-PASSWORD http://YOUR-LOGGING-SERVER-IP:9200/_cat/indices/esp32-logs-*?v
```

**Troubleshooting:**
- If connection fails: Verify Elasticsearch is running
- If authentication fails: Check username/password
- If template exists: This is OK, it will be updated

### Step 2: Kibana Configuration (5 minutes)

```bash
./setup-kibana.sh
```

**Verify:**
- [ ] Script completes without errors
- [ ] Index pattern created
- [ ] 8 visualizations created
- [ ] Dashboard created

**Validation commands:**
```bash
# Check index pattern
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:5601/api/saved_objects/index-pattern/esp32-logs-pattern

# List visualizations
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:5601/api/saved_objects/_find?type=visualization

# Check dashboard
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:5601/api/saved_objects/dashboard/esp32-dashboard
```

**Manual verification:**
- [ ] Open Kibana in browser: http://YOUR-LOGGING-SERVER-IP:5601
- [ ] Navigate to Stack Management > Index Patterns
- [ ] Verify "esp32-logs-*" exists
- [ ] Navigate to Visualizations
- [ ] Verify 8 visualizations exist (ESP32 - ...)
- [ ] Navigate to Dashboards
- [ ] Verify "ESP32 Smoker Monitoring" exists

**Troubleshooting:**
- If Kibana not ready: Wait 30 seconds and retry
- If objects already exist: This is OK, they will be updated
- If 404 errors: Ensure Elasticsearch setup completed first

### Step 3: Alert Configuration (2 minutes)

```bash
./setup-watchers.sh
```

**Verify:**
- [ ] Script completes without errors
- [ ] esp32-alerts index created
- [ ] 4 watchers created

**Validation commands:**
```bash
# Check watchers
curl -u elastic:YOUR-ELASTIC-PASSWORD http://YOUR-LOGGING-SERVER-IP:9200/_watcher/stats?pretty

# List all watchers
curl -u elastic:YOUR-ELASTIC-PASSWORD "http://YOUR-LOGGING-SERVER-IP:9200/_watcher/watch/_all" | grep -o '"_id":"[^"]*"'

# Check alerts index
curl -u elastic:YOUR-ELASTIC-PASSWORD http://YOUR-LOGGING-SERVER-IP:9200/_cat/indices/esp32-alerts?v
```

**Expected watchers:**
- [ ] esp32-error-state
- [ ] esp32-temp-range
- [ ] esp32-sensor-failures
- [ ] esp32-high-error-rate

**Troubleshooting:**
- If X-Pack warning: Watchers require X-Pack (Elastic Stack license)
- If watchers already exist: This is OK
- If watcher creation fails: Check Elasticsearch logs

### Step 4: Cribl Configuration (10 minutes)

Follow manual steps in `logging/cribl/README.md`:

**4.1 Create Syslog Source**
- [ ] Navigate to Cribl: http://YOUR-LOGGING-SERVER-IP:9000
- [ ] Login with credentials
- [ ] Go to Data > Sources
- [ ] Add Syslog source
  - Input ID: syslog_udp
  - Protocol: UDP
  - Port: 9514
  - Format: RFC 5424
- [ ] Save and enable

**4.2 Create Elasticsearch Destination**
- [ ] Go to Data > Destinations
- [ ] Add Elasticsearch destination
  - Output ID: elasticsearch_output
  - URL: http://YOUR-LOGGING-SERVER-IP:9200
  - Auth: Basic (elastic / YOUR-ELASTIC-PASSWORD)
  - Index: `esp32-logs-%{+YYYY.MM.dd}`
  - Pipeline: esp32-syslog-pipeline
- [ ] Test connection (should succeed)
- [ ] Save

**4.3 Create Route**
- [ ] Go to Data > Routes
- [ ] Add route
  - Name: esp32_route
  - Filter: `sourcetype=='syslog' && (appname=='smoker' || host.includes('ESP32'))`
  - Output: elasticsearch_output
- [ ] Save
- [ ] Ensure route is enabled

**Validation:**
- [ ] Check Cribl Live Data
- [ ] Verify syslog source shows status: Active
- [ ] Verify route shows status: Active

### Step 5: Pipeline Testing (2 minutes)

```bash
./test-logging.sh
```

**Verify:**
- [ ] Script sends 50+ test messages
- [ ] No "connection refused" errors
- [ ] Messages sent successfully

**Validation in Cribl:**
- [ ] Go to Monitoring > Live Data
- [ ] Filter by source: syslog_udp
- [ ] Should see incoming test messages
- [ ] Messages should show appname: smoker

**Validation in Elasticsearch:**
```bash
# Wait 10 seconds for indexing, then check
sleep 10

# Count documents
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  "http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-*/_count?pretty"

# Should return count > 0

# View sample documents
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  "http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-*/_search?pretty&size=5&sort=@timestamp:desc"
```

**Expected results:**
- [ ] Document count > 50
- [ ] Documents have @timestamp field
- [ ] Documents have device_name: ESP32Smoker
- [ ] Documents have parsed fields (temperature, severity, etc.)
- [ ] No "parse_failure" tags

**Validation in Kibana:**
- [ ] Open dashboard: http://YOUR-LOGGING-SERVER-IP:5601/app/dashboards
- [ ] Open "ESP32 Smoker Monitoring"
- [ ] Adjust time range to "Last 15 minutes"
- [ ] Verify visualizations show data:
  - [ ] Log Level Distribution shows pie slices
  - [ ] Logs Over Time shows data points
  - [ ] Error Rate shows a number
  - [ ] Temperature Timeline shows data (if temp logs sent)
  - [ ] Recent Errors table has rows

**Troubleshooting:**
- If no data in Elasticsearch: Check Cribl Live Data
- If data in ES but not Kibana: Refresh index pattern in Kibana
- If parsing errors: Check ingest pipeline configuration

### Step 6: ESP32 Configuration (5 minutes)

**Update ESP32 code:**
```cpp
// In your syslog configuration
#define SYSLOG_SERVER "YOUR-LOGGING-SERVER-IP"
#define SYSLOG_PORT 9514
#define SYSLOG_DEVICE_HOSTNAME "ESP32Smoker"  // Or your device name
#define SYSLOG_APP_NAME "smoker"              // Or your app name
```

**Verify format:**
```cpp
// Ensure you're using RFC 5424 format
// <priority>1 timestamp hostname appname procid msgid sdata message
```

**Deploy to ESP32:**
- [ ] Compile code
- [ ] Upload to ESP32
- [ ] Connect to WiFi
- [ ] Monitor serial output for syslog send confirmations

**Validation:**
- [ ] Check Cribl Live Data for messages from your ESP32
- [ ] Check Kibana dashboard for real device data
- [ ] Verify device_name matches your ESP32
- [ ] Verify all log types appear (temp, state, etc.)

## Post-Deployment Verification

### End-to-End Test

1. **Generate a complete cycle of logs from ESP32:**
   - [ ] Temperature readings
   - [ ] State transitions (IDLE → STARTUP → RUNNING)
   - [ ] PID control values
   - [ ] Relay status changes
   - [ ] At least one warning
   - [ ] At least one error (optional, for testing)

2. **Verify in Cribl:**
   - [ ] All messages appear in Live Data
   - [ ] All messages route to Elasticsearch

3. **Verify in Elasticsearch:**
   - [ ] All messages indexed
   - [ ] All fields parsed correctly
   - [ ] No parse failures

4. **Verify in Kibana:**
   - [ ] Dashboard updates in real-time
   - [ ] All visualizations show data
   - [ ] Filters work correctly
   - [ ] Time range adjustment works

5. **Verify Alerts:**
   ```bash
   # Trigger a test alert (send critical error)
   echo "<130>1 $(date -u +"%Y-%m-%dT%H:%M:%S.%3NZ") ESP32Smoker smoker $$ - - State transition: RUNNING -> ERROR" \
     | nc -u YOUR-LOGGING-SERVER-IP 9514

   # Wait 2 minutes, then check alerts
   sleep 120
   curl -u elastic:YOUR-ELASTIC-PASSWORD http://YOUR-LOGGING-SERVER-IP:9200/esp32-alerts/_search?pretty
   ```
   - [ ] Alert appears in esp32-alerts index
   - [ ] Alert has correct alert_type
   - [ ] Alert has timestamp and details

### Performance Check

- [ ] Dashboard loads in < 5 seconds
- [ ] Searches complete in < 1 second
- [ ] No errors in Elasticsearch logs
- [ ] No errors in Kibana logs
- [ ] No errors in Cribl logs
- [ ] CPU usage normal on all systems
- [ ] Memory usage normal on all systems
- [ ] Disk usage reasonable (check with `df -h`)

### Documentation Review

- [ ] All team members can access documentation
- [ ] README.md is clear and comprehensive
- [ ] QUICK_REFERENCE.md has correct URLs and credentials
- [ ] Troubleshooting steps are effective

## Maintenance Setup

### Daily Tasks
- [ ] Set up daily dashboard check (manual or automated)
- [ ] Subscribe to critical alerts (when email configured)

### Weekly Tasks
- [ ] Schedule weekly review of error patterns
- [ ] Check disk space: `df -h /var/lib/elasticsearch`

### Monthly Tasks
- [ ] Schedule monthly index cleanup:
  ```bash
  # Delete indices older than 30 days
  # Create a cron job or manual calendar reminder
  ```
- [ ] Review alert thresholds for accuracy
- [ ] Update visualizations if needed

## Rollback Plan

If deployment fails or causes issues:

### Rollback Elasticsearch
```bash
# Delete index template
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X DELETE http://YOUR-LOGGING-SERVER-IP:9200/_index_template/esp32-logs

# Delete ingest pipeline
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X DELETE http://YOUR-LOGGING-SERVER-IP:9200/_ingest/pipeline/esp32-syslog-pipeline

# Delete indices
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X DELETE "http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-*"

# Delete watchers
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X DELETE http://YOUR-LOGGING-SERVER-IP:9200/_watcher/watch/esp32-error-state
# ... repeat for other watchers
```

### Rollback Kibana
```bash
# Delete dashboard
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X DELETE http://YOUR-LOGGING-SERVER-IP:5601/api/saved_objects/dashboard/esp32-dashboard

# Delete visualizations
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X DELETE "http://YOUR-LOGGING-SERVER-IP:5601/api/saved_objects/visualization/esp32-viz-*"

# Delete index pattern
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X DELETE http://YOUR-LOGGING-SERVER-IP:5601/api/saved_objects/index-pattern/esp32-logs-pattern
```

### Rollback Cribl
- Manually delete source, destination, and route via UI
- Or reset Cribl configuration to previous state

### Rollback ESP32
- Remove syslog configuration from code
- Re-deploy previous firmware version

## Success Criteria

Deployment is successful when:

- [ ] **All setup scripts complete without errors**
- [ ] **Test messages appear in Kibana dashboard**
- [ ] **Real ESP32 logs flow through entire pipeline**
- [ ] **All visualizations display data correctly**
- [ ] **At least one alert has triggered successfully**
- [ ] **Dashboard loads in < 5 seconds**
- [ ] **No errors in any system logs**
- [ ] **Team members can access and understand the system**

## Sign-Off

Deployment completed by: _____________________ Date: _____________________

Issues encountered: _______________________________________________________

_________________________________________________________________________

Workarounds applied: ______________________________________________________

_________________________________________________________________________

Post-deployment notes: ____________________________________________________

_________________________________________________________________________

## Next Steps After Deployment

1. [ ] Train team on dashboard usage
2. [ ] Document any environment-specific customizations
3. [ ] Set up email/Slack notifications for alerts
4. [ ] Create runbook for common operational tasks
5. [ ] Schedule first monthly review
6. [ ] Plan adaptation for next ESP32 project

---

**Note:** Keep this checklist for future reference and for deploying to additional environments.
