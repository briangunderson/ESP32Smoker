# Verify ESP32 logs are routing to correct index
$esUrl = "http://YOUR-LOGGING-SERVER-IP:9200"
$creds = New-Object PSCredential "elastic", (ConvertTo-SecureString "YOUR-ELASTIC-PASSWORD" -AsPlainText -Force)

Write-Host "Checking Elasticsearch indices..." -ForegroundColor Green
Write-Host ""

# Check for esp32-logs indices
Write-Host "ESP32 log indices:" -ForegroundColor Cyan
try {
    $response = Invoke-RestMethod -Uri "$esUrl/_cat/indices/esp32-logs-*?v&h=index,docs.count,store.size&s=index:desc" -Credential $creds
    Write-Host $response
    Write-Host ""
}
catch {
    Write-Host "No esp32-logs-* indices found yet" -ForegroundColor Yellow
}

# Check for logs-cribl indices (should not have ESP32 data)
Write-Host "Cribl default indices (ESP32 should NOT be here):" -ForegroundColor Cyan
try {
    $response = Invoke-RestMethod -Uri "$esUrl/_cat/indices/logs-cribl*?v&h=index,docs.count,store.size&s=index:desc" -Credential $creds
    Write-Host $response
    Write-Host ""
}
catch {
    Write-Host "No logs-cribl indices found" -ForegroundColor Yellow
}

# Search for recent ESP32 logs
Write-Host "Recent ESP32 logs (last 5):" -ForegroundColor Cyan
try {
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
                            gte = "now-5m"
                        }
                    }
                }
            }
        }
    } | ConvertTo-Json -Depth 10

    $response = Invoke-RestMethod -Uri "$esUrl/esp32-logs-*/_search" -Method Post -Credential $creds -ContentType "application/json" -Body $searchBody

    if ($response.hits.total.value -gt 0) {
        Write-Host "  Found $($response.hits.total.value) logs in last 5 minutes" -ForegroundColor Green
        Write-Host ""
        foreach ($hit in $response.hits.hits) {
            $source = $hit._source
            Write-Host "  [$($source.'@timestamp')] $($source.severity): $($source.message.Substring(0, [Math]::Min(80, $source.message.Length)))" -ForegroundColor White
        }
    }
    else {
        Write-Host "  No logs found in last 5 minutes" -ForegroundColor Yellow
        Write-Host "  Make sure ESP32 is running and Cribl route is configured" -ForegroundColor Yellow
    }
}
catch {
    Write-Host "  Error searching logs: $_" -ForegroundColor Red
}

Write-Host ""
Write-Host "Verification complete!" -ForegroundColor Green
