# Check latest temperature log from ESP32
$esUrl = "http://192.168.1.97:9200"
$creds = New-Object PSCredential "elastic", (ConvertTo-SecureString "N5l2MV3c" -AsPlainText -Force)

Write-Host "Searching for latest MAX31865 temperature log..." -ForegroundColor Green
Write-Host ""

$searchBody = @{
    size = 1
    sort = @(@{
        "@timestamp" = @{
            order = "desc"
        }
    })
    query = @{
        bool = @{
            must = @(
                @{
                    term = @{
                        "_dataId.keyword" = "logs-syslog-9543"
                    }
                }
                @{
                    wildcard = @{
                        message = "*MAX31865*"
                    }
                }
            )
        }
    }
} | ConvertTo-Json -Depth 10

try {
    $response = Invoke-RestMethod -Uri "$esUrl/logs-cribl/_search" -Method Post -Credential $creds -ContentType "application/json" -Body $searchBody

    if ($response.hits.total.value -gt 0) {
        $source = $response.hits.hits[0]._source

        Write-Host "Latest MAX31865 log:" -ForegroundColor Cyan
        Write-Host "  Timestamp:   $($source.'@timestamp')" -ForegroundColor White
        Write-Host "  app_name:    $($source.app_name)" -ForegroundColor $(if ($source.app_name) { "Green" } else { "Red" })
        Write-Host "  device_name: $($source.device_name)" -ForegroundColor $(if ($source.device_name) { "Green" } else { "Red" })
        Write-Host "  priority:    $($source.priority)" -ForegroundColor $(if ($source.priority) { "Green" } else { "Red" })
        Write-Host "  severity:    $($source.severity)" -ForegroundColor $(if ($source.severity) { "Green" } else { "Red" })
        Write-Host "  tags:        $($source.tags -join ', ')" -ForegroundColor Yellow

        $msg = $source.message
        if ($msg -and $msg.Length -gt 0) {
            Write-Host "  message:     $($msg.Substring(0, [Math]::Min(120, $msg.Length)))" -ForegroundColor Gray
        }

        Write-Host ""

        if ($source.app_name -eq "smoker" -and $source.device_name -eq "ESP32Smoker") {
            Write-Host "SUCCESS! Pipeline is working correctly!" -ForegroundColor Green
        } else {
            Write-Host "Pipeline NOT working - fields not extracted" -ForegroundColor Red
        }
    }
    else {
        Write-Host "No MAX31865 logs found" -ForegroundColor Yellow
    }
}
catch {
    Write-Host "Error: $_" -ForegroundColor Red
}
