# Check for successfully parsed ESP32 logs (without parse_failure tag)
$esUrl = "http://192.168.1.97:9200"
$creds = New-Object PSCredential "elastic", (ConvertTo-SecureString "N5l2MV3c" -AsPlainText -Force)

Write-Host "Searching for successfully parsed ESP32 logs..." -ForegroundColor Green
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
                    term = @{
                        "_dataId.keyword" = "logs-syslog-9543"
                    }
                }
            )
            must_not = @(
                @{
                    term = @{
                        "tags.keyword" = "parse_failure"
                    }
                }
            )
        }
    }
} | ConvertTo-Json -Depth 10

try {
    $response = Invoke-RestMethod -Uri "$esUrl/logs-cribl/_search" -Method Post -Credential $creds -ContentType "application/json" -Body $searchBody

    if ($response.hits.total.value -gt 0) {
        Write-Host "Found $($response.hits.total.value) successfully parsed logs!" -ForegroundColor Green
        Write-Host ""

        foreach ($hit in $response.hits.hits) {
            $source = $hit._source
            Write-Host "[@$($source.'@timestamp')]" -ForegroundColor Cyan
            Write-Host "  app_name:    $($source.app_name)" -ForegroundColor White
            Write-Host "  device_name: $($source.device_name)" -ForegroundColor White
            Write-Host "  severity:    $($source.severity)" -ForegroundColor White
            Write-Host "  priority:    $($source.priority)" -ForegroundColor White

            if ($source.message -and $source.message.Length -gt 0) {
                $msg = $source.message.Substring(0, [Math]::Min(100, $source.message.Length))
                Write-Host "  message:     $msg" -ForegroundColor Gray
            }

            Write-Host ""
        }
    }
    else {
        Write-Host "No successfully parsed logs found" -ForegroundColor Yellow
        Write-Host "All ESP32 logs have parse_failure tag" -ForegroundColor Yellow
    }
}
catch {
    Write-Host "Error: $_" -ForegroundColor Red
}
