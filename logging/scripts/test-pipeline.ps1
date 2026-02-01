# Test the ESP32 syslog pipeline parsing
$esUrl = "http://YOUR-LOGGING-SERVER-IP:9200"
$creds = New-Object PSCredential "elastic", (ConvertTo-SecureString "YOUR-ELASTIC-PASSWORD" -AsPlainText -Force)

Write-Host "Testing ESP32 Syslog Pipeline..." -ForegroundColor Green
Write-Host ""

# Test message from the error log (using simple ASCII for compatibility)
$testMessage = "<7>1 - ESP32Smoker smoker - - - [MAX31865] RAW=0x28ED (10477) | R=1374.85 | T=95.91C (204.6F)"

$testBody = @{
    docs = @(
        @{
            _source = @{
                message = $testMessage
            }
        }
    )
} | ConvertTo-Json -Depth 5

Write-Host "Test message:" -ForegroundColor Cyan
Write-Host "  $testMessage" -ForegroundColor White
Write-Host ""

try {
    $response = Invoke-RestMethod -Uri "$esUrl/_ingest/pipeline/esp32-syslog-pipeline/_simulate" -Method Post -Credential $creds -ContentType "application/json" -Body $testBody

    if ($response.docs[0].doc._source) {
        Write-Host "Pipeline parsing successful!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Parsed fields:" -ForegroundColor Cyan

        $doc = $response.docs[0].doc._source

        Write-Host "  priority:    $($doc.priority)" -ForegroundColor White
        Write-Host "  severity:    $($doc.severity)" -ForegroundColor White
        Write-Host "  device_name: $($doc.device_name)" -ForegroundColor White
        Write-Host "  app_name:    $($doc.app_name)" -ForegroundColor White
        Write-Host "  message:     $($doc.message)" -ForegroundColor White

        if ($doc.temperature) {
            Write-Host "  temperature: $($doc.temperature) F" -ForegroundColor Green
        }

        Write-Host ""
    }
    else {
        Write-Host "Pipeline parsing failed" -ForegroundColor Red
        if ($response.docs[0].error) {
            Write-Host "Error: $($response.docs[0].error.reason)" -ForegroundColor Red
        }
    }
}
catch {
    Write-Host "Error testing pipeline: $_" -ForegroundColor Red
}

Write-Host ""
Write-Host "Pipeline test complete!" -ForegroundColor Green

