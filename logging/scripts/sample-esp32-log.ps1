# Get a sample ESP32 log from logs-cribl
$esUrl = "http://192.168.1.97:9200"
$creds = New-Object PSCredential "elastic", (ConvertTo-SecureString "N5l2MV3c" -AsPlainText -Force)

Write-Host "Fetching sample ESP32 log..." -ForegroundColor Green
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
            )
            filter = @{
                range = @{
                    "@timestamp" = @{
                        gte = "now-60m"
                    }
                }
            }
        }
    }
} | ConvertTo-Json -Depth 10

try {
    $response = Invoke-RestMethod -Uri "$esUrl/logs-cribl/_search" -Method Post -Credential $creds -ContentType "application/json" -Body $searchBody

    if ($response.hits.total.value -gt 0) {
        Write-Host "Sample log document:" -ForegroundColor Cyan
        Write-Host ""

        $source = $response.hits.hits[0]._source

        # Display all relevant fields
        Write-Host "Timestamp:     $($source.'@timestamp')" -ForegroundColor White
        Write-Host "_dataId:       $($source._dataId)" -ForegroundColor Yellow
        Write-Host "message:       $($source.message)" -ForegroundColor White
        Write-Host "priority:      $($source.priority)" -ForegroundColor White
        Write-Host "severity:      $($source.severity)" -ForegroundColor White
        Write-Host "device_name:   $($source.device_name)" -ForegroundColor White
        Write-Host "app_name:      $($source.app_name)" -ForegroundColor White
        Write-Host "log_message:   $($source.log_message)" -ForegroundColor Gray
        Write-Host ""

        if ($source.tags) {
            Write-Host "tags:          $($source.tags -join ', ')" -ForegroundColor Magenta
        }

        Write-Host ""
        Write-Host "Full document (JSON):" -ForegroundColor Cyan
        $source | ConvertTo-Json -Depth 5
    }
    else {
        Write-Host "No recent logs found" -ForegroundColor Red
    }
}
catch {
    Write-Host "Error: $_" -ForegroundColor Red
}
