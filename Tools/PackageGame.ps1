# Package a Shipping build of Reinforcement Forge for Windows (Steam depot layout).
param(
    [string]$Engine = "D:\UE_5.8",
    [string]$Project = "D:\UEprojects\BrainForge\BrainForge.uproject",
    [string]$Out = "D:\UEprojects\BrainForge\Packaged"
)

& "$Engine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
    -project="$Project" `
    -noP4 -platform=Win64 `
    -clientconfig=Shipping `
    -cook -build -stage -pak -archive `
    -archivedirectory="$Out" `
    -prereqs

Write-Host "Packaged to $Out\Windows"
