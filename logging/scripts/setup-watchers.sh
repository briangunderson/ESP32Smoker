#!/bin/bash
# Setup script for Elasticsearch Watchers (Alerts)
# Run this script AFTER setup-elasticsearch.sh

set -e

# Configuration
ES_HOST="${ES_HOST:-YOUR-LOGGING-SERVER-IP}"
ES_PORT="${ES_PORT:-9200}"
ES_USER="${ES_USER:-elastic}"
ES_PASS="${ES_PASS:-YOUR-ELASTIC-PASSWORD}"
ES_URL="http://${ES_HOST}:${ES_PORT}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_DIR="$(dirname "$SCRIPT_DIR")/elasticsearch"

echo "========================================"
echo "ESP32 Logging Infrastructure Setup"
echo "Elasticsearch Watchers (Alerts)"
echo "========================================"
echo ""
echo "Elasticsearch URL: ${ES_URL}"
echo ""

# Function to make authenticated requests
es_request() {
    local method=$1
    local endpoint=$2
    local data=$3

    if [ -n "$data" ]; then
        curl -s -X "${method}" \
            -u "${ES_USER}:${ES_PASS}" \
            -H "Content-Type: application/json" \
            "${ES_URL}/${endpoint}" \
            -d "@${data}"
    else
        curl -s -X "${method}" \
            -u "${ES_USER}:${ES_PASS}" \
            -H "Content-Type: application/json" \
            "${ES_URL}/${endpoint}"
    fi
}

# Check if X-Pack is available
echo "Step 1: Checking X-Pack availability..."
if ! es_request GET "_xpack" | grep -q "available.*true"; then
    echo "WARNING: X-Pack may not be available or enabled"
    echo "Watchers require X-Pack. If this is an OSS installation, use Kibana Alerting instead."
    echo ""
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi
echo ""

# Create esp32-alerts index
echo "Step 2: Creating 'esp32-alerts' index for alert storage..."
RESPONSE=$(es_request PUT "esp32-alerts" <<EOF
{
  "settings": {
    "number_of_shards": 1,
    "number_of_replicas": 1
  },
  "mappings": {
    "properties": {
      "alert_type": {"type": "keyword"},
      "timestamp": {"type": "date"},
      "device_name": {"type": "keyword"},
      "message": {"type": "text"},
      "severity": {"type": "keyword"},
      "temperature": {"type": "float"},
      "error_count": {"type": "integer"},
      "failure_count": {"type": "integer"}
    }
  }
}
EOF
)
if echo "$RESPONSE" | grep -q "acknowledged.*true"; then
    echo "✓ Alerts index created"
else
    echo "WARNING: Failed to create alerts index (it may already exist)"
fi
echo ""

# Create watchers
echo "Step 3: Creating watchers..."

echo "  - Creating 'Error State' watcher..."
RESPONSE=$(es_request PUT "_watcher/watch/esp32-error-state" "${CONFIG_DIR}/watcher-error-state.json")
if echo "$RESPONSE" | grep -q "created.*true"; then
    echo "  ✓ Created"
else
    echo "  WARNING: May already exist or failed to create"
fi

echo "  - Creating 'Temperature Out of Range' watcher..."
RESPONSE=$(es_request PUT "_watcher/watch/esp32-temp-range" "${CONFIG_DIR}/watcher-temperature-out-of-range.json")
if echo "$RESPONSE" | grep -q "created.*true"; then
    echo "  ✓ Created"
else
    echo "  WARNING: May already exist or failed to create"
fi

echo "  - Creating 'Consecutive Sensor Failures' watcher..."
RESPONSE=$(es_request PUT "_watcher/watch/esp32-sensor-failures" "${CONFIG_DIR}/watcher-consecutive-sensor-failures.json")
if echo "$RESPONSE" | grep -q "created.*true"; then
    echo "  ✓ Created"
else
    echo "  WARNING: May already exist or failed to create"
fi

echo "  - Creating 'High Error Rate' watcher..."
RESPONSE=$(es_request PUT "_watcher/watch/esp32-high-error-rate" "${CONFIG_DIR}/watcher-high-error-rate.json")
if echo "$RESPONSE" | grep -q "created.*true"; then
    echo "  ✓ Created"
else
    echo "  WARNING: May already exist or failed to create"
fi

echo ""

# List all watchers
echo "Step 4: Verifying watchers..."
es_request GET "_watcher/watch/_all" | grep -o '"_id":"[^"]*"' | sed 's/"_id":"/  - /' | sed 's/"$//'
echo ""

echo "========================================"
echo "Watchers setup complete!"
echo "========================================"
echo ""
echo "Configured alerts:"
echo "  1. Error State Alert - Triggers when device enters ERROR state"
echo "  2. Temperature Range Alert - Triggers when temp < 50°F or > 500°F"
echo "  3. Sensor Failures Alert - Triggers on 3+ sensor errors in 5 minutes"
echo "  4. High Error Rate Alert - Triggers on 10+ errors in 15 minutes"
echo ""
echo "Alert actions:"
echo "  - Log to Elasticsearch logs"
echo "  - Index to 'esp32-alerts' index"
echo ""
echo "View alerts in Kibana:"
echo "  - Index pattern: 'esp32-alerts'"
echo "  - Or check Elasticsearch logs"
echo ""
echo "To add email/Slack notifications:"
echo "  1. Configure email/Slack settings in Elasticsearch"
echo "  2. Edit watcher JSON files to add email/webhook actions"
echo "  3. Re-run this script"
echo ""
