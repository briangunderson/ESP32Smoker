#!/bin/bash
# Setup script for Elasticsearch index template and ingest pipeline
# Run this script to configure Elasticsearch for ESP32 logging

set -e

# Configuration
ES_HOST="${ES_HOST:-192.168.1.97}"
ES_PORT="${ES_PORT:-9200}"
ES_USER="${ES_USER:-elastic}"
ES_PASS="${ES_PASS:-N5l2MV3c}"
ES_URL="http://${ES_HOST}:${ES_PORT}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_DIR="$(dirname "$SCRIPT_DIR")/elasticsearch"

echo "========================================"
echo "ESP32 Logging Infrastructure Setup"
echo "Elasticsearch Configuration"
echo "========================================"
echo ""
echo "Elasticsearch URL: ${ES_URL}"
echo "User: ${ES_USER}"
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

# Check Elasticsearch connection
echo "Step 1: Checking Elasticsearch connection..."
if ! es_request GET "" > /dev/null 2>&1; then
    echo "ERROR: Cannot connect to Elasticsearch at ${ES_URL}"
    echo "Please check that Elasticsearch is running and credentials are correct."
    exit 1
fi
echo "✓ Connected to Elasticsearch"
echo ""

# Create ingest pipeline
echo "Step 2: Creating ingest pipeline 'esp32-syslog-pipeline'..."
RESPONSE=$(es_request PUT "_ingest/pipeline/esp32-syslog-pipeline" "${CONFIG_DIR}/ingest-pipeline.json")
if echo "$RESPONSE" | grep -q "acknowledged.*true"; then
    echo "✓ Ingest pipeline created successfully"
else
    echo "Response: $RESPONSE"
    echo "WARNING: Failed to create ingest pipeline (it may already exist)"
fi
echo ""

# Create index template
echo "Step 3: Creating index template for 'esp32-logs-*'..."
RESPONSE=$(es_request PUT "_index_template/esp32-logs" "${CONFIG_DIR}/index-template.json")
if echo "$RESPONSE" | grep -q "acknowledged.*true"; then
    echo "✓ Index template created successfully"
else
    echo "Response: $RESPONSE"
    echo "WARNING: Failed to create index template (it may already exist)"
fi
echo ""

# Create initial index (optional, will be created automatically on first log)
echo "Step 4: Creating initial index 'esp32-logs-$(date +%Y.%m.%d)'..."
INDEX_NAME="esp32-logs-$(date +%Y.%m.%d)"
RESPONSE=$(es_request PUT "${INDEX_NAME}" <<EOF
{
  "settings": {
    "number_of_shards": 1,
    "number_of_replicas": 1
  }
}
EOF
)
if echo "$RESPONSE" | grep -q "acknowledged.*true"; then
    echo "✓ Initial index created: ${INDEX_NAME}"
else
    echo "Response: $RESPONSE"
    echo "WARNING: Failed to create initial index (it may already exist)"
fi
echo ""

# Verify setup
echo "Step 5: Verifying setup..."
echo "Checking ingest pipeline..."
if es_request GET "_ingest/pipeline/esp32-syslog-pipeline" | grep -q "esp32-syslog-pipeline"; then
    echo "✓ Ingest pipeline exists"
else
    echo "✗ Ingest pipeline NOT found"
fi

echo "Checking index template..."
if es_request GET "_index_template/esp32-logs" | grep -q "esp32-logs"; then
    echo "✓ Index template exists"
else
    echo "✗ Index template NOT found"
fi

echo "Checking indices..."
es_request GET "_cat/indices/esp32-logs-*?v" | head -10
echo ""

echo "========================================"
echo "Elasticsearch setup complete!"
echo "========================================"
echo ""
echo "Next steps:"
echo "1. Run setup-kibana.sh to configure Kibana"
echo "2. Run setup-watchers.sh to configure alerts"
echo "3. Configure Cribl (see logging/cribl/README.md)"
echo "4. Start your ESP32 device and verify logs appear"
echo ""
