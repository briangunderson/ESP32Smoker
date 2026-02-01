#!/bin/bash
# Test script to send sample syslog messages to Cribl
# Use this to verify the logging pipeline is working

set -e

# Configuration
CRIBL_HOST="${CRIBL_HOST:-YOUR-LOGGING-SERVER-IP}"
CRIBL_PORT="${CRIBL_PORT:-9514}"
DEVICE_NAME="${DEVICE_NAME:-ESP32Smoker}"
APP_NAME="${APP_NAME:-smoker}"

echo "========================================"
echo "ESP32 Logging Test Script"
echo "========================================"
echo ""
echo "Sending test syslog messages to:"
echo "  Host: ${CRIBL_HOST}"
echo "  Port: ${CRIBL_PORT}"
echo "  Device: ${DEVICE_NAME}"
echo "  App: ${APP_NAME}"
echo ""

# Function to send syslog message
# Args: priority, message
send_syslog() {
    local priority=$1
    local message=$2
    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%S.%3NZ")
    local pid=$$

    # RFC 5424 format: <priority>version timestamp hostname app-name procid msgid structured-data msg
    local syslog_msg="<${priority}>1 ${timestamp} ${DEVICE_NAME} ${APP_NAME} ${pid} - - ${message}"

    echo "Sending: $message"
    echo "$syslog_msg" | nc -u -w1 "${CRIBL_HOST}" "${CRIBL_PORT}"
    sleep 0.5
}

# Calculate priorities (facility * 8 + severity)
# Facility 16 = local0, Severities: 7=debug, 6=info, 4=warning, 3=error, 2=critical
FACILITY=16
PRI_DEBUG=$((FACILITY * 8 + 7))    # 135
PRI_INFO=$((FACILITY * 8 + 6))     # 134
PRI_WARNING=$((FACILITY * 8 + 4))  # 132
PRI_ERROR=$((FACILITY * 8 + 3))    # 131
PRI_CRIT=$((FACILITY * 8 + 2))     # 130

echo "Test 1: Basic system messages"
send_syslog $PRI_INFO "System starting up"
send_syslog $PRI_INFO "WiFi connected"
send_syslog $PRI_DEBUG "Debug mode enabled"

echo ""
echo "Test 2: Temperature readings"
send_syslog $PRI_INFO "[Temperature] Current: 225.5°F, Setpoint: 225.0°F"
send_syslog $PRI_INFO "[Temperature] Current: 228.2°F, Setpoint: 225.0°F"
send_syslog $PRI_INFO "[Temperature] Current: 222.8°F, Setpoint: 225.0°F"

echo ""
echo "Test 3: State transitions"
send_syslog $PRI_INFO "State transition: IDLE -> STARTUP"
send_syslog $PRI_INFO "Entering state: STARTUP"
send_syslog $PRI_INFO "State transition: STARTUP -> RUNNING"
send_syslog $PRI_INFO "Entering state: RUNNING"

echo ""
echo "Test 4: PID control"
send_syslog $PRI_DEBUG "[PID] P=2.5, I=0.8, D=0.1, Output=45.2"
send_syslog $PRI_DEBUG "[PID] P=1.8, I=0.9, D=-0.2, Output=42.1"
send_syslog $PRI_DEBUG "[PID] P=-0.5, I=1.0, D=-0.3, Output=38.5"

echo ""
echo "Test 5: Relay control"
send_syslog $PRI_INFO "[Relay] Auger: on, Fan: on, Igniter: off"
send_syslog $PRI_INFO "[Relay] Auger: off, Fan: on, Igniter: off"
send_syslog $PRI_INFO "[Relay] Auger: on, Fan: on, Igniter: on"

echo ""
echo "Test 6: Warning messages"
send_syslog $PRI_WARNING "Temperature approaching upper limit"
send_syslog $PRI_WARNING "Auger interlock triggered - fan not running"

echo ""
echo "Test 7: Error messages"
send_syslog $PRI_ERROR "Sensor read error"
send_syslog $PRI_ERROR "MAX31865 fault detected"
send_syslog $PRI_ERROR "Consecutive sensor failures: 2"

echo ""
echo "Test 8: Critical alerts (for testing watchers)"
send_syslog $PRI_CRIT "State transition: RUNNING -> ERROR"
send_syslog $PRI_CRIT "Temperature out of range: 505.3°F"
send_syslog $PRI_CRIT "Emergency shutdown triggered"

echo ""
echo "Test 9: Multiple device simulation"
DEVICE_NAME="ESP32Smoker2"
send_syslog $PRI_INFO "[Temperature] Current: 250.0°F, Setpoint: 250.0°F"
send_syslog $PRI_INFO "Entering state: RUNNING"

echo ""
echo "========================================"
echo "Test messages sent!"
echo "========================================"
echo ""
echo "Next steps:"
echo "1. Check Cribl Live Data: http://${CRIBL_HOST}:9000/monitoring/live-data"
echo "2. Check Kibana Dashboard: http://${CRIBL_HOST}:5601/app/dashboards"
echo "3. Search for logs in Kibana Discover"
echo "4. Wait 1-2 minutes and check for alerts in 'esp32-alerts' index"
echo ""
echo "To query Elasticsearch directly:"
echo "  curl -u elastic:YOUR-ELASTIC-PASSWORD http://${CRIBL_HOST}:9200/esp32-logs-*/_search?pretty"
echo ""
