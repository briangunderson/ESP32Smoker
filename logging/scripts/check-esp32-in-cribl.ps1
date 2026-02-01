# Check for ESP32 logs in the logs-cribl index
$esUrl = "http://192.168.1.97:9200"
$creds = New-Object PSCredential "elastic", (ConvertTo-SecureString "N5l2MV3c" -AsPlainText -Force)

Write-Host "Searching for ESP32 logs in logs-cribl index..." -ForegroundColor Green
Write-Host ""

$searchBody = @{
    size = 5
    sort = @(@{
        "@timestamp" = @{
            order = "desc"
        }
    })
    query = @{
        bool = @{
            must = @(
                @{
                    match = @{
                        app_name = "smoker"
                    }
                }
            )
            filter = @{
                range = @{
                    "@timestamp" = @{
                        gte = "now-10m"
                    }
                }
            }
        }
    }
} | ConvertTo-Json -Depth 10

try {
    $response = Invoke-RestMethod -Uri "$esUrl/logs-cribl/_search" -Method Post -Credential $creds -ContentType "application/json" -Body $searchBody

    if ($response.hits.total.value -gt 0) {
        Write-Host "Found $($response.hits.total.value) ESP32 logs in last 10 minutes" -ForegroundColor Green
        Write-Host ""

        foreach ($hit in $response.hits.hits) {
            $source = $hit._source
            Write-Host "[@$($source.'@timestamp')]" -ForegroundColor Cyan
            Write-Host "  Severity:    $($source.severity)" -ForegroundColor White
            Write-Host "  Device:      $($source.device_name)" -ForegroundColor White
            Write-Host "  App:         $($source.app_name)" -ForegroundColor White
            Write-Host "  _dataId:     $($source._dataId)" -ForegroundColor Yellow

            if ($source.log_message) {
                $msg = $source.log_message
                if ($msg.Length -gt 100) {
                    $msg = $msg.Substring(0, 100) + "..."
                }
                Write-Host "  Message:     $msg" -ForegroundColor Gray
            }

            if ($source.temperature) {
                Write-Host "  Temperature: $($source.temperature) F" -ForegroundColor Green
            }

            Write-Host ""
        }
    }
    else {
        Write-Host "No ESP32 logs found in last 10 minutes" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Checking if ANY logs exist with _dataId=logs-syslog-9543..." -ForegroundColor Cyan

        $dataIdBody = @{
            size = 5
            query = @{
                bool = @{
                    must = @(
                        @{
                            match = @{
                                _dataId = "logs-syslog-9543"
                            }
                        }
                    )
                    filter = @{
                        range = @{
                            "@timestamp" = @{
                                gte = "now-10m"
                            }
                        }
                    }
                }
            }
        } | ConvertTo-Json -Depth 10

        $dataIdResponse = Invoke-RestMethod -Uri "$esUrl/logs-cribl/_search" -Method Post -Credential $creds -ContentType "application/json" -Body $dataIdBody

        if ($dataIdResponse.hits.total.value -gt 0) {
            Write-Host "Found $($dataIdResponse.hits.total.value) logs with _dataId=logs-syslog-9543" -ForegroundColor Green
        }
        else {
            Write-Host "No logs with _dataId=logs-syslog-9543 found" -ForegroundColor Red
            Write-Host "Check Cribl configuration to ensure _dataId is being set for port 9543" -ForegroundColor Yellow
        }
    }
}
catch {
    Write-Host "Error searching: $_" -ForegroundColor Red
}

Write-Host ""
Write-Host "Check complete!" -ForegroundColor Green
