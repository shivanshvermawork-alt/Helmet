param(
    [string]$portName = "COM7",
    [int]$baudRate = 9600
)

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "Live Serial Monitor for $portName ($baudRate baud)" -ForegroundColor Cyan
Write-Host "(Press Ctrl+C to stop)" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# Create SerialPort without hardware flow control (RTS)
$port = New-Object System.IO.Ports.SerialPort($portName, $baudRate, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$port.ReadTimeout = 1000
$port.WriteTimeout = 1000

try {
    Write-Host "Connecting to $portName..." -ForegroundColor Cyan
    $port.Open()
    
    # Pulse DTR to reset Arduino and start clean stream
    $port.DtrEnable = $true
    Start-Sleep -Milliseconds 500
    
    Write-Host "Connected to $portName! Streaming live data:`n" -ForegroundColor Yellow

    while ($true) {
        try {
            $line = $port.ReadLine()
            if ($line -and $line.Trim().Length -gt 0) {
                Write-Host $line.Trim() -ForegroundColor Green
            }
        }
        catch [System.TimeoutException] {
            # Check for any partial data
            $raw = $port.ReadExisting()
            if ($raw -and $raw.Trim().Length -gt 0) {
                Write-Host $raw.Trim() -ForegroundColor Green
            }
        }
    }
}
catch {
    Write-Host "`nError accessing $portName : $_" -ForegroundColor Red
    Write-Host "If port is busy, close any other terminal tab or Serial Monitor and retry." -ForegroundColor Yellow
}
finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        $port.Dispose()
    }
    Write-Host "`nSerial monitor closed." -ForegroundColor Cyan
}
