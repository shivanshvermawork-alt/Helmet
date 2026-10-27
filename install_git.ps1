$targetDir = "$env:USERPROFILE\.git-bin"
if (Test-Path "$targetDir\cmd\git.exe") {
    Write-Host "Git already extracted at $targetDir"
    & "$targetDir\cmd\git.exe" --version
    exit 0
}

New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
$zipPath = "$targetDir\mingit.zip"
Write-Host "Downloading MinGit..."
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri "https://github.com/git-for-windows/git/releases/download/v2.44.0.windows.1/MinGit-2.44.0-64-bit.zip" -OutFile $zipPath

Write-Host "Extracting MinGit..."
Expand-Archive -Path $zipPath -DestinationPath $targetDir -Force
Remove-Item $zipPath -Force

Write-Host "MinGit ready:"
& "$targetDir\cmd\git.exe" --version
