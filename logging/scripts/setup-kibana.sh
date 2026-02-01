#!/bin/bash
# Setup script for Kibana index pattern, visualizations, and dashboard
# Run this script AFTER setup-elasticsearch.sh

set -e

# Configuration
KIBANA_HOST="${KIBANA_HOST:-YOUR-LOGGING-SERVER-IP}"
KIBANA_PORT="${KIBANA_PORT:-5601}"
ES_USER="${ES_USER:-elastic}"
ES_PASS="${ES_PASS:-YOUR-ELASTIC-PASSWORD}"
KIBANA_URL="http://${KIBANA_HOST}:${KIBANA_PORT}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_DIR="$(dirname "$SCRIPT_DIR")/kibana"

echo "========================================"
echo "ESP32 Logging Infrastructure Setup"
echo "Kibana Configuration"
echo "========================================"
echo ""
echo "Kibana URL: ${KIBANA_URL}"
echo ""

# Function to make authenticated requests to Kibana
kibana_request() {
    local method=$1
    local endpoint=$2
    local data=$3

    if [ -n "$data" ]; then
        curl -s -X "${method}" \
            -u "${ES_USER}:${ES_PASS}" \
            -H "kbn-xsrf: true" \
            -H "Content-Type: application/json" \
            "${KIBANA_URL}/${endpoint}" \
            -d "@${data}"
    else
        curl -s -X "${method}" \
            -u "${ES_USER}:${ES_PASS}" \
            -H "kbn-xsrf: true" \
            "${KIBANA_URL}/${endpoint}"
    fi
}

# Check Kibana connection
echo "Step 1: Checking Kibana connection..."
if ! kibana_request GET "api/status" > /dev/null 2>&1; then
    echo "ERROR: Cannot connect to Kibana at ${KIBANA_URL}"
    echo "Please check that Kibana is running and credentials are correct."
    exit 1
fi
echo "✓ Connected to Kibana"
echo ""

# Wait for Kibana to be ready
echo "Step 2: Waiting for Kibana to be ready..."
MAX_RETRIES=30
RETRY_COUNT=0
while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    if kibana_request GET "api/status" | grep -q "available"; then
        echo "✓ Kibana is ready"
        break
    fi
    RETRY_COUNT=$((RETRY_COUNT + 1))
    echo "Waiting for Kibana... ($RETRY_COUNT/$MAX_RETRIES)"
    sleep 2
done

if [ $RETRY_COUNT -eq $MAX_RETRIES ]; then
    echo "ERROR: Kibana did not become ready in time"
    exit 1
fi
echo ""

# Create index pattern
echo "Step 3: Creating index pattern 'esp32-logs-*'..."
RESPONSE=$(kibana_request POST "api/saved_objects/index-pattern/esp32-logs-pattern" "${CONFIG_DIR}/index-pattern.json")
if echo "$RESPONSE" | grep -q "esp32-logs"; then
    echo "✓ Index pattern created successfully"
else
    echo "Response: $RESPONSE"
    echo "WARNING: Failed to create index pattern (it may already exist)"
fi
echo ""

# Create visualizations
echo "Step 4: Creating visualizations..."

echo "  - Creating 'Log Level Distribution' visualization..."
kibana_request POST "api/saved_objects/visualization/esp32-viz-log-level-distribution" \
    "${CONFIG_DIR}/viz-log-level-distribution.json" > /dev/null

echo "  - Creating 'Logs Over Time' visualization..."
kibana_request POST "api/saved_objects/visualization/esp32-viz-logs-over-time" \
    "${CONFIG_DIR}/viz-logs-over-time.json" > /dev/null

echo "  - Creating 'Error Rate' visualization..."
kibana_request POST "api/saved_objects/visualization/esp32-viz-error-rate" \
    "${CONFIG_DIR}/viz-error-rate.json" > /dev/null

echo "  - Creating 'Device Status' visualization..."
kibana_request POST "api/saved_objects/visualization/esp32-viz-device-status" \
    "${CONFIG_DIR}/viz-device-status.json" > /dev/null

echo "  - Creating 'Temperature Timeline' visualization..."
kibana_request POST "api/saved_objects/visualization/esp32-viz-temperature-timeline" \
    "${CONFIG_DIR}/viz-temperature-timeline.json" > /dev/null

echo "  - Creating 'PID Performance' visualization..."
kibana_request POST "api/saved_objects/visualization/esp32-viz-pid-performance" \
    "${CONFIG_DIR}/viz-pid-performance.json" > /dev/null

echo "  - Creating 'Recent Errors' visualization..."
kibana_request POST "api/saved_objects/visualization/esp32-viz-recent-errors" \
    "${CONFIG_DIR}/viz-recent-errors.json" > /dev/null

echo "  - Creating 'State Transitions' visualization..."
kibana_request POST "api/saved_objects/visualization/esp32-viz-state-transitions" \
    "${CONFIG_DIR}/viz-state-transitions.json" > /dev/null

echo "✓ All visualizations created"
echo ""

# Create dashboard
echo "Step 5: Creating dashboard 'ESP32 Smoker Monitoring'..."
RESPONSE=$(kibana_request POST "api/saved_objects/dashboard/esp32-dashboard" "${CONFIG_DIR}/dashboard.json")
if echo "$RESPONSE" | grep -q "esp32"; then
    echo "✓ Dashboard created successfully"
else
    echo "Response: $RESPONSE"
    echo "WARNING: Failed to create dashboard (it may already exist)"
fi
echo ""

echo "========================================"
echo "Kibana setup complete!"
echo "========================================"
echo ""
echo "Access your dashboard at:"
echo "${KIBANA_URL}/app/dashboards"
echo ""
echo "Dashboard name: 'ESP32 Smoker Monitoring'"
echo ""
echo "Next steps:"
echo "1. Run setup-watchers.sh to configure alerts"
echo "2. Configure Cribl (see logging/cribl/README.md)"
echo "3. Start your ESP32 device and verify data appears"
echo ""
