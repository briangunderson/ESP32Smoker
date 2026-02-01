# PowerShell script to import Kibana visualizations and dashboard
# Run this from PowerShell

$kibanaUrl = "http://192.168.1.97:5601"
$credentials = "elastic:N5l2MV3c"
$encodedCreds = [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes($credentials))
$headers = @{
    "kbn-xsrf" = "true"
    "Authorization" = "Basic $encodedCreds"
}

$scriptPath = $PSScriptRoot
$kibanaPath = Join-Path (Split-Path $scriptPath) "kibana"

Write-Host "Importing Kibana visualizations and dashboard..." -ForegroundColor Green

# Import visualizations
$vizFiles = @(
    "viz-error-rate.json",
    "viz-device-status.json",
    "viz-log-level-distribution.json",
    "viz-logs-over-time.json",
    "viz-temperature-timeline.json",
    "viz-pid-performance.json",
    "viz-recent-errors.json",
    "viz-state-transitions.json"
)

foreach ($vizFile in $vizFiles) {
    $filePath = Join-Path $kibanaPath $vizFile
    $vizId = $vizFile -replace "^viz-", "esp32-viz-" -replace ".json$", ""

    Write-Host "Importing $vizFile as $vizId..." -ForegroundColor Cyan

    try {
        $jsonContent = Get-Content $filePath -Raw | ConvertFrom-Json
        $body = $jsonContent | ConvertTo-Json -Depth 100

        $response = Invoke-RestMethod -Uri "$kibanaUrl/api/saved_objects/visualization/$vizId`?overwrite=true" `
            -Method Post `
            -Headers $headers `
            -ContentType "application/json" `
            -Body $body

        Write-Host "  ✓ Successfully imported $vizId" -ForegroundColor Green
    }
    catch {
        Write-Host "  ✗ Failed to import $vizFile : $_" -ForegroundColor Red
    }
}

# Import dashboard
Write-Host "`nImporting dashboard..." -ForegroundColor Cyan
try {
    $dashboardFile = Join-Path $kibanaPath "dashboard.json"
    $jsonContent = Get-Content $dashboardFile -Raw | ConvertFrom-Json
    $body = $jsonContent | ConvertTo-Json -Depth 100

    $response = Invoke-RestMethod -Uri "$kibanaUrl/api/saved_objects/dashboard/esp32-smoker-dashboard?overwrite=true" `
        -Method Post `
        -Headers $headers `
        -ContentType "application/json" `
        -Body $body

    Write-Host "  ✓ Successfully imported dashboard" -ForegroundColor Green
}
catch {
    Write-Host "  ✗ Failed to import dashboard: $_" -ForegroundColor Red
}

Write-Host "`nKibana import complete!" -ForegroundColor Green
Write-Host "Visit: $kibanaUrl/app/dashboards" -ForegroundColor Yellow
