# ESP32 Logging Infrastructure - Quick Reference

## URLs

| Service | URL | Credentials |
|---------|-----|-------------|
| Cribl | http://YOUR-LOGGING-SERVER-IP:9000/ | claude / YOUR-CRIBL-PASSWORD |
| Kibana | http://YOUR-LOGGING-SERVER-IP:5601/ | elastic / YOUR-ELASTIC-PASSWORD |
| Elasticsearch | http://YOUR-LOGGING-SERVER-IP:9200/ | elastic / YOUR-ELASTIC-PASSWORD |

## Quick Setup (First Time)

```bash
cd logging/scripts

# 1. Setup Elasticsearch (5 min)
./setup-elasticsearch.sh

# 2. Setup Kibana (5 min)
./setup-kibana.sh

# 3. Setup Alerts (2 min)
./setup-watchers.sh

# 4. Configure Cribl manually (10 min)
#    Follow: logging/cribl/README.md

# 5. Test everything (2 min)
./test-logging.sh
```

## ESP32 Configuration

**Syslog Settings:**
- Host: `YOUR-LOGGING-SERVER-IP`
- Port: `9514` (UDP)
- Format: RFC 5424
- Device Name: `ESP32Smoker` (or your device name)
- App Name: `smoker` (or your app name)

**Priority Calculation:**
```cpp
// Priority = Facility * 8 + Severity
#define LOG_FACILITY 16  // local0
#define LOG_INFO     6
#define LOG_WARNING  4
#define LOG_ERROR    3
#define LOG_CRIT     2

int priority = (LOG_FACILITY * 8) + LOG_INFO;  // = 134
```

## Common Commands

### Check Logs in Elasticsearch
```bash
# Count logs today
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-*/_count?pretty

# Recent logs
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  "http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-*/_search?pretty&size=10&sort=@timestamp:desc"

# Search for errors
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  "http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-*/_search?pretty&q=severity:error"
```

### Check Alerts
```bash
# View all alerts
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:9200/esp32-alerts/_search?pretty

# Count alerts today
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  "http://YOUR-LOGGING-SERVER-IP:9200/esp32-alerts/_count?pretty&q=timestamp:>now-1d"
```

### Manage Indices
```bash
# List all ESP32 indices
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:9200/_cat/indices/esp32-logs-*?v

# Delete old index (example: Jan 1st)
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X DELETE http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-2026.01.01
```

### Test Syslog
```bash
# Send test message
echo "<134>1 $(date -u +"%Y-%m-%dT%H:%M:%S.%3NZ") ESP32Smoker smoker $$ - - Test message" \
  | nc -u YOUR-LOGGING-SERVER-IP 9514

# Or use the test script
cd logging/scripts
./test-logging.sh
```

## Dashboard Access

**Main Dashboard:**
http://YOUR-LOGGING-SERVER-IP:5601/app/dashboards#/view/esp32-dashboard

**Discover (Search Logs):**
http://YOUR-LOGGING-SERVER-IP:5601/app/discover

**Visualizations:**
http://YOUR-LOGGING-SERVER-IP:5601/app/visualize

## Log Message Formats

### Temperature
```
[Temperature] Current: 225.5°F, Setpoint: 225.0°F
```
Extracts: `temperature`, `setpoint`

### PID Control
```
[PID] P=2.5, I=0.8, D=0.1, Output=45.2
```
Extracts: `pid_p`, `pid_i`, `pid_d`, `pid_output`

### State
```
State transition: STARTUP -> RUNNING
```
Extracts: `state`

### Relays
```
[Relay] Auger: on, Fan: on, Igniter: off
```
Extracts: `relay_auger`, `relay_fan`, `relay_igniter`

## Troubleshooting

### No data in Kibana?
1. Check Cribl Live Data: http://YOUR-LOGGING-SERVER-IP:9000/monitoring/live-data
2. Verify ESP32 is sending to correct IP/port
3. Check firewall allows UDP 9514
4. Run test-logging.sh to verify pipeline

### Parsing errors?
1. Check for parse failures:
   ```bash
   curl -u elastic:YOUR-ELASTIC-PASSWORD \
     "http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-*/_search?pretty&q=tags:parse_failure"
   ```
2. Verify log format matches RFC 5424
3. Test ingest pipeline with sample data

### Alerts not firing?
1. Check watcher status:
   ```bash
   curl -u elastic:YOUR-ELASTIC-PASSWORD \
     http://YOUR-LOGGING-SERVER-IP:9200/_watcher/stats?pretty
   ```
2. Manually trigger watcher:
   ```bash
   curl -u elastic:YOUR-ELASTIC-PASSWORD \
     -X POST "http://YOUR-LOGGING-SERVER-IP:9200/_watcher/watch/esp32-error-state/_execute?pretty"
   ```

## Key Files

| File | Purpose |
|------|---------|
| `elasticsearch/index-template.json` | Field mappings |
| `elasticsearch/ingest-pipeline.json` | Log parsing rules |
| `kibana/dashboard.json` | Main dashboard config |
| `cribl/pipeline-config.yml` | Cribl pipeline (reference) |
| `scripts/setup-elasticsearch.sh` | One-time ES setup |
| `scripts/test-logging.sh` | Test the pipeline |

## Alerts Configured

1. **Error State** - Device enters ERROR state
2. **Temperature Range** - Temp < 50°F or > 500°F
3. **Sensor Failures** - 3+ errors in 5 minutes
4. **High Error Rate** - 10+ errors in 15 minutes

## Index Fields

### Standard
- `@timestamp`, `device_name`, `app_name`
- `severity`, `facility`, `message`
- `priority`, `host`, `pid`, `tags`

### Extracted
- `temperature`, `setpoint`
- `pid_p`, `pid_i`, `pid_d`, `pid_output`
- `state`, `relay_auger`, `relay_fan`, `relay_igniter`
- `error_code`, `log_type`

## Reusing for Other Projects

1. **Copy directory**: `cp -r logging/ ../NewProject/`
2. **Rename indices**: Replace `esp32-logs` with `yourproject-logs`
3. **Update patterns**: Edit `ingest-pipeline.json` for your log formats
4. **Update fields**: Edit `index-template.json` for your data
5. **Recreate visualizations**: Use existing as templates
6. **Run setup scripts**: Execute all setup-*.sh scripts

## Performance Tips

### High Volume (Many Devices)
- Increase Cribl bulk size: `maxPayloadEvents: 1000`
- Increase shards: `"number_of_shards": 3`
- Slower refresh: `"index.refresh_interval": "10s"`

### Low Resources (Small Server)
- Disable replicas: `"number_of_replicas": 0`
- Slower refresh: `"index.refresh_interval": "30s"`
- Delete old indices regularly

## Maintenance

**Daily:** Check dashboard, review alerts
**Weekly:** Check disk space, review errors
**Monthly:** Delete old indices, optimize queries

**Delete indices older than 30 days:**
```bash
# List indices
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  http://YOUR-LOGGING-SERVER-IP:9200/_cat/indices/esp32-logs-*?v

# Delete old one
curl -u elastic:YOUR-ELASTIC-PASSWORD \
  -X DELETE http://YOUR-LOGGING-SERVER-IP:9200/esp32-logs-2025.12.01
```

## Support

1. Check `logging/README.md` for detailed docs
2. Review Cribl/ES/Kibana logs
3. Run `test-logging.sh` to verify setup
4. Check component-specific README files
