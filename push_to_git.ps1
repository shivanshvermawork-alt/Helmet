Write-Host "Staging files..."
git add .

Write-Host "Committing changes..."
git commit -m "feat: AegisHelm Smart IoT Motorcycle Dashboard & Safety Interlock System"

Write-Host "Pushing to origin/aniket..."
git push origin aniket

Write-Host "Current Status:"
git status
