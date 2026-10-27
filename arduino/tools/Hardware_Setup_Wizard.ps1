# ======================================================================
#  🛡️ AEGISHELM SMART HELMET SYSTEM - HARDWARE SETUP & LAUNCH WIZARD
# ======================================================================

Clear-Host
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host "   AEGISHELM: SMART HELMET MOTORCYCLE PROTECTION & MONITORING         " -ForegroundColor Green
Write-Host "               Hardware Auto-Detect & Launch Wizard                   " -ForegroundColor Yellow
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host ""

# 1. Check Backend Server on Port 8080
Write-Host "[1/4] Checking AegisHelm Gateway Server (Port 8080)..." -NoNewline
$serverActive = $false
try {
    $res = Invoke-WebRequest -Uri "http://localhost:8080/api/ports" -UseBasicParsing -TimeoutSec 2 -ErrorAction Stop
    if ($res.StatusCode -eq 200) { $serverActive = $true }
} catch {
    $serverActive = $false
}

if ($serverActive) {
    Write-Host " [ACTIVE]" -ForegroundColor Green
} else {
    Write-Host " [STARTING]" -ForegroundColor Yellow
    $serverDir = Resolve-Path "$PSScriptRoot\..\..\server"
    Start-Process -FilePath "cmd.exe" -ArgumentList "/c cd `"$serverDir`" && node server.js" -WindowStyle Minimized
    Start-Sleep -Seconds 3
    Write-Host "      Backend Gateway launched on http://localhost:8080" -ForegroundColor Green
}

Write-Host ""

# 2. Hardware COM Port Detection
Write-Host "[2/4] Scanning for Connected Arduino Hardware..." -ForegroundColor Cyan

function Get-ArduinoPorts {
    try {
        $apiRes = Invoke-WebRequest -Uri "http://localhost:8080/api/ports" -UseBasicParsing -TimeoutSec 2 -ErrorAction SilentlyContinue
        if ($apiRes) {
            $json = $apiRes.Content | ConvertFrom-Json
            return $json.ports
        }
    } catch {}
    
    # Fallback to direct .NET ports
    $raw = [System.IO.Ports.SerialPort]::GetPortNames()
    $res = @()
    foreach ($p in $raw) {
        $res += [PSCustomObject]@{ path = $p; friendlyName = "Serial Port ($p)" }
    }
    return $res
}

$ports = Get-ArduinoPorts
$spinner = @('|', '/', '-', '\')
$spinIdx = 0

if (-not $ports -or $ports.Count -eq 0) {
    Write-Host "      No Arduino detected yet." -ForegroundColor Yellow
    Write-Host "      --> Please plug your Arduino Receiver into any USB port..." -ForegroundColor White
    
    while (-not $ports -or $ports.Count -eq 0) {
        Write-Host -NoNewline "`r      Waiting for USB connection $($spinner[$spinIdx % 4])  "
        $spinIdx++
        Start-Sleep -Milliseconds 500
        $ports = Get-ArduinoPorts
    }
    Write-Host "`r                                                             `r" -NoNewline
}

Write-Host "      Found $($ports.Count) serial device(s):" -ForegroundColor Green
$selectedPort = $null
for ($i = 0; $i -lt $ports.Count; $i++) {
    $p = $ports[$i]
    $name = if ($p.friendlyName) { $p.friendlyName } else { $p.path }
    Write-Host "      [$($i+1)] $($p.path) - $name" -ForegroundColor Yellow
    if ($i -eq 0) { $selectedPort = $p.path }
}

# Auto-connect first port to backend
if ($selectedPort) {
    Write-Host ""
    Write-Host "[3/4] Connecting $selectedPort to AegisHelm Web Gateway..." -NoNewline -ForegroundColor Cyan
    try {
        $postData = @{ port = $selectedPort } | ConvertTo-Json
        $connectRes = Invoke-WebRequest -Uri "http://localhost:8080/api/connect" -Method Post -Body $postData -ContentType "application/json" -UseBasicParsing -TimeoutSec 3 -ErrorAction SilentlyContinue
        Write-Host " [LINKED]" -ForegroundColor Green
    } catch {
        Write-Host " [AUTO-LINK PENDING]" -ForegroundColor Yellow
    }
}

# 4. Open Live Dashboard
Write-Host ""
Write-Host "[4/4] Opening AegisHelm Live Cockpit in your browser..." -ForegroundColor Cyan
Start-Process "http://localhost:8080"
Start-Sleep -Seconds 1
Write-Host "      Live Cockpit: http://localhost:8080" -ForegroundColor Green

Write-Host ""
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host "                 HARDWARE FIRMWARE CHECKLIST                          " -ForegroundColor Yellow
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host " 1. HELMET TRANSMITTER (Battery/Power Bank Powered):" -ForegroundColor White
Write-Host "    Open & Upload: arduino/helmet_transmitter/helmet_transmitter.ino" -ForegroundColor Gray
Write-Host "    (Sensors: IR1->D2, IR2->D3, MQ3->A0, MPU6050->A4/A5, RF_TX->D12)" -ForegroundColor DarkGray
Write-Host ""
Write-Host " 2. BIKE RECEIVER (Connected to PC via USB):" -ForegroundColor White
Write-Host "    Open & Upload: arduino/Receiver/Receiver.ino" -ForegroundColor Gray
Write-Host "    (Relay->D8 Active LOW, Buzzer->D3, GreenLED->D9, RF_RX->D11)" -ForegroundColor DarkGray
Write-Host ""
Write-Host " * Tip: If Arduino IDE shows 'Access Denied' when uploading, click the" -ForegroundColor Yellow
Write-Host "        'UPLOAD MODE: Release Port' button in the dashboard top bar." -ForegroundColor Yellow
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host ""

$choice = ReadHost "Press [M] to open Live Terminal Monitor, or [Enter] to finish and keep dashboard open"
if ($choice -eq 'M' -or $choice -eq 'm') {
    if ($selectedPort) {
        & "$PSScriptRoot\monitor.ps1" -portName $selectedPort -baudRate 9600
    }
}
