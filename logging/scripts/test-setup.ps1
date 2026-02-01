# Test Elasticsearch/Kibana setup
$esUrl = "http://YOUR-LOGGING-SERVER-IP:9200"
$kibanaUrl = "http://YOUR-LOGGING-SERVER-IP:5601"
$creds = "elastic:YOUR-ELASTIC-PASSWORD"

Write-Host "Testing Elasticsearch/Kibana Setup..." -ForegroundColor Green
Write-Host ""

# Test 1: Elasticsearch is running
Write-Host "Test 1: Elasticsearch connection..." -NoNewline
try {
    $response = Invoke-RestMethod -Uri "$esUrl" -Credential (New-Object PSCredential "elastic", (ConvertTo-SecureString "YOUR-ELASTIC-PASSWORD" -AsPlainText -Force))
    Write-Host " ✓ PASS" -ForegroundColor Green
    Write-Host "  Version: $($response.version.number)"
}
catch {
    Write-Host " ✗ FAIL" -ForegroundColor Red
}

# Test 2: Ingest pipeline exists
Write-Host "Test 2: Ingest pipeline 'esp32-syslog-pipeline'..." -NoNewline
try {
    $response = Invoke-RestMethod -Uri "$esUrl/_ingest/pipeline/esp32-syslog-pipeline" -Credential (New-Object PSCredential "elastic", (ConvertTo-SecureString "YOUR-ELASTIC-PASSWORD" -AsPlainText -Force))
    Write-Host " ✓ PASS" -ForegroundColor Green
}
catch {
    Write-Host " ✗ FAIL" -ForegroundColor Red
}

# Test 3: Index template exists
Write-Host "Test 3: Index template 'esp32-logs-template'..." -NoNewline
try {
    $response = Invoke-RestMethod -Uri "$esUrl/_index_template/esp32-logs-template" -Credential (New-Object PSCredential "elastic", (ConvertTo-SecureString "YOUR-ELASTIC-PASSWORD" -AsPlainText -Force))
    Write-Host " ✓ PASS" -ForegroundColor Green
}
catch {
    Write-Host " ✗ FAIL" -ForegroundColor Red
}

# Test 4: Kibana is running
Write-Host "Test 4: Kibana connection..." -NoNewline
try {
    $response = Invoke-RestMethod -Uri "$kibanaUrl/api/status" -Credential (New-Object PSCredential "elastic", (ConvertTo-SecureString "YOUR-ELASTIC-PASSWORD" -AsPlainText -Force))
    Write-Host " ✓ PASS" -ForegroundColor Green
    Write-Host "  Status: $($response.status.overall.level)"
}
catch {
    Write-Host " ✗ FAIL" -ForegroundColor Red
}

Write-Host ""
Write-Host "Setup verification complete!" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. ESP32 is now configured to send logs to Cribl (UDP 9514)"
Write-Host "2. Cribl should forward logs to Elasticsearch"
Write-Host "3. Create visualizations in Kibana at: $kibanaUrl"
Write-Host "4. Or import them manually from: d:\localrepos\ESP32Smoker\logging\kibana\"
