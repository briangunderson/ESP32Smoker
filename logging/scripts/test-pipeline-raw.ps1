# Test the ESP32 syslog pipeline with _raw field
$esUrl = "http://192.168.1.97:9200"
$creds = New-Object PSCredential "elastic", (ConvertTo-SecureString "N5l2MV3c" -AsPlainText -Force)

Write-Host "Testing ESP32 Syslog Pipeline with _raw field..." -ForegroundColor Green
Write-Host ""

# Test message from actual log (using _raw field)
$testMessage = "<7>1 - ESP32Smoker smoker - - - [MAX31865] RAW=0x28EB (10475) | R=1374.59 | RefR=4300 | T=95.84C (204.5F)"

$testBody = @{
    docs = @(
        @{
            _source = @{
                _raw = $testMessage
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
        Write-Host "  log_message: $($doc.log_message)" -ForegroundColor White

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
