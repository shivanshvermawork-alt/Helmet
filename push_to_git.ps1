$git = "$env:USERPROFILE\.git-bin\cmd\git.exe"

Write-Host "Configuring Git..."
& $git config --global user.name "shivanshvermawork-alt"
& $git config --global user.email "shivanshverma.work@gmail.com"

if (-not (Test-Path ".git")) {
    Write-Host "Initializing Git repository..."
    & $git init
}

Write-Host "Adding remote origin..."
& $git remote remove origin 2>$null
& $git remote add origin "https://github.com/shivanshvermawork-alt/Helmet.git"

Write-Host "Staging files..."
& $git add .

Write-Host "Committing files..."
& $git commit -m "Initial commit: Dual-Arduino Smart Helmet and Bike Ignition System"

Write-Host "Setting main branch..."
& $git branch -M main

Write-Host "Current Status:"
& $git status
