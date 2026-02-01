# Cribl Configuration for ESP32 Syslog

## Overview
This directory contains Cribl Stream configuration for receiving and processing syslog messages from ESP32 devices.

## Configuration Files
- `pipeline-config.yml` - Complete Cribl pipeline configuration

## Manual Configuration Steps

Since Cribl configuration is typically done through the web UI, here are the steps to manually configure Cribl:

### 1. Configure Syslog Source

1. Navigate to **Data > Sources** in Cribl UI (http://YOUR-LOGGING-SERVER-IP:9000/)
2. Click **Add Source**
3. Select **Syslog**
4. Configure:
   - **Input ID**: `syslog_udp`
   - **Protocol**: UDP
   - **Port**: 9514
   - **Host**: 0.0.0.0
   - **Format**: RFC 5424
   - **Description**: "UDP syslog receiver for ESP32 devices on port 9514"
5. Click **Save**

### 2. Configure Elasticsearch Destination

1. Navigate to **Data > Destinations**
2. Click **Add Destination**
3. Select **Elasticsearch**
4. Configure:
   - **Output ID**: `elasticsearch_output`
   - **Elasticsearch URL**: http://YOUR-LOGGING-SERVER-IP:9200
   - **Auth Type**: Basic
   - **Username**: elastic
   - **Password**: YOUR-ELASTIC-PASSWORD
   - **Index**: `esp32-logs-%{+YYYY.MM.dd}` (time-based index)
   - **Pipeline**: `esp32-syslog-pipeline` (the ingest pipeline)
   - **Bulk API Version**: v2
   - **Compression**: gzip
   - **Max Payload Size**: 4096 KB
   - **Flush Period**: 1 second
5. Click **Save**

### 3. Create Processing Pipeline

1. Navigate to **Processing > Pipelines**
2. Click **Add Pipeline**
3. Name: `esp32_processing`
4. Add the following functions in order:

#### Function 1: Parse Priority
- Type: **Eval**
- Filter: `true`
- Add Fields:
  - `facility` = `Math.floor(priority / 8)`
  - `severity_num` = `priority % 8`

#### Function 2: Add Severity Name
- Type: **Eval**
- Filter: `true`
- Add Field:
  - `severity` = `const s = ['emergency', 'alert', 'critical', 'error', 'warning', 'notice', 'info', 'debug']; s[severity_num] || 'unknown'`

#### Function 3: Extract Temperature
- Type: **Regex Extract**
- Filter: `message.match(/temperature/i)`
- Source Field: `message`
- Regex: `/(?:Current:|Temperature:)\s*(\d+(?:\.\d+)?)°F/i`
- Extract: `temperature` from capture group 1

#### Function 4: Extract Setpoint
- Type: **Regex Extract**
- Filter: `message.match(/setpoint/i)`
- Source Field: `message`
- Regex: `/Setpoint:\s*(\d+(?:\.\d+)?)°F/i`
- Extract: `setpoint` from capture group 1

#### Function 5: Add Tags
- Type: **Eval**
- Filter: `true`
- Add Field:
  - `tags` = `['esp32', 'iot', 'syslog', appname].filter(Boolean)`

5. Click **Save Pipeline**

### 4. Create Route

1. Navigate to **Data > Routes**
2. Click **Add Route**
3. Configure:
   - **Route Name**: `esp32_route`
   - **Filter**: `sourcetype=='syslog' && (appname=='smoker' || host.includes('ESP32'))`
   - **Pipeline**: `esp32_processing`
   - **Output**: `elasticsearch_output`
4. Click **Save**

### 5. Set Default Route

1. Ensure there's a default route that catches all other traffic
2. Configure it to output to `devnull` or another destination

## Testing the Configuration

1. Restart your ESP32 device
2. Check Cribl Live Data: **Monitoring > Live Data**
3. Filter by source: `syslog_udp`
4. You should see incoming syslog messages
5. Check that they're being routed to Elasticsearch
6. Verify in Kibana that data is appearing in `esp32-logs-*` indices

## Troubleshooting

### No data appearing in Cribl
- Check firewall rules allow UDP 9514
- Verify ESP32 is sending to correct IP (YOUR-LOGGING-SERVER-IP:9514)
- Check Cribl status: **Monitoring > System > Status**

### Data not reaching Elasticsearch
- Test Elasticsearch connection from Cribl UI
- Check Elasticsearch credentials
- Verify ingest pipeline exists in Elasticsearch
- Check Cribl logs: **Monitoring > Logs**

### Parsing errors
- Check Live Data to see raw messages
- Adjust regex patterns in pipeline functions
- Test regex patterns against sample messages

## Alternative: Import Configuration

If Cribl supports configuration import:

1. Navigate to **Settings > Global Settings > Configuration**
2. Look for import/export options
3. Import the `pipeline-config.yml` file

Note: Cribl's configuration format may vary by version. The YAML file provided is a reference - adjust as needed for your Cribl version.
