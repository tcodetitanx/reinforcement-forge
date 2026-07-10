# Reinforcement Forge - sprite pipeline.
# Chroma-keys the magenta AI sheet backgrounds to transparency and crops the
# icons the game uses at runtime (Content/RawAssets/Sprites).
param(
    [string]$ImagesDir = "D:\UEprojects\BrainForge\images",
    [string]$OutDir = "D:\UEprojects\BrainForge\Content\RawAssets\Sprites"
)

Add-Type -AssemblyName System.Drawing
New-Item -ItemType Directory -Force $OutDir | Out-Null

function Key-And-Save {
    param(
        [System.Drawing.Bitmap]$Sheet,
        [int]$CX, [int]$CY, [int]$W, [int]$H,
        [string]$OutPath,
        [double]$NearDist = 55.0,
        [double]$FarDist = 150.0
    )
    $x0 = [Math]::Max(0, $CX - [int]($W / 2))
    $y0 = [Math]::Max(0, $CY - [int]($H / 2))
    $W = [Math]::Min($W, $Sheet.Width - $x0)
    $H = [Math]::Min($H, $Sheet.Height - $y0)

    # key color = corner pixel of the sheet
    $key = $Sheet.GetPixel(2, 2)
    $kr = [double]$key.R; $kg = [double]$key.G; $kb = [double]$key.B

    $out = New-Object System.Drawing.Bitmap($W, $H, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)

    $srcRect = New-Object System.Drawing.Rectangle($x0, $y0, $W, $H)
    $srcData = $Sheet.LockBits($srcRect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $len = $srcData.Stride * $H
    $buf = New-Object byte[] $len
    [System.Runtime.InteropServices.Marshal]::Copy($srcData.Scan0, $buf, 0, $len)
    $Sheet.UnlockBits($srcData)

    $dstData = $out.LockBits((New-Object System.Drawing.Rectangle(0, 0, $W, $H)), [System.Drawing.Imaging.ImageLockMode]::WriteOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $obuf = New-Object byte[] ($dstData.Stride * $H)

    for ($y = 0; $y -lt $H; $y++) {
        $row = $y * $srcData.Stride
        $orow = $y * $dstData.Stride
        for ($x = 0; $x -lt $W; $x++) {
            $i = $row + $x * 4
            $b = [double]$buf[$i]; $g = [double]$buf[$i + 1]; $r = [double]$buf[$i + 2]
            $dist = [Math]::Sqrt(($r - $kr) * ($r - $kr) + ($g - $kg) * ($g - $kg) + ($b - $kb) * ($b - $kb))
            $a = 0.0
            if ($dist -ge $FarDist) { $a = 1.0 }
            elseif ($dist -gt $NearDist) { $a = ($dist - $NearDist) / ($FarDist - $NearDist) }

            # despill: pull magenta fringe (high R+B, low G) back toward neutral
            if ($r -gt $g -and $b -gt $g) {
                $excess = [Math]::Min($r, $b) - $g
                $r = [Math]::Max(0.0, $r - $excess * 0.65)
                $b = [Math]::Max(0.0, $b - $excess * 0.35)
            }

            $o = $orow + $x * 4
            $obuf[$o] = [byte][Math]::Round($b)
            $obuf[$o + 1] = [byte][Math]::Round($g)
            $obuf[$o + 2] = [byte][Math]::Round($r)
            $obuf[$o + 3] = [byte][Math]::Round($a * 255.0)
        }
    }
    [System.Runtime.InteropServices.Marshal]::Copy($obuf, 0, $dstData.Scan0, $obuf.Length)
    $out.UnlockBits($dstData)
    $out.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $out.Dispose()
    Write-Host "wrote $OutPath"
}

$sheets = Get-ChildItem "$ImagesDir\*.png" | Sort-Object Name
# (1)=UI kit, (2)=node types, (3)=panels, (4)=effects, (5)=misc icons
$uiKit  = New-Object System.Drawing.Bitmap($sheets[0].FullName)
$nodes  = New-Object System.Drawing.Bitmap($sheets[1].FullName)

# --- node type icons (sheet 2, top rows) ---
Key-And-Save $nodes  62 100  74  74 "$OutDir\node_input.png"
Key-And-Save $nodes 236 100  74  74 "$OutDir\node_relay.png"
Key-And-Save $nodes 413  99  74  74 "$OutDir\node_threshold.png"
Key-And-Save $nodes 615 100  74  74 "$OutDir\node_memory.png"
Key-And-Save $nodes 819  99  78  78 "$OutDir\node_reward.png"
Key-And-Save $nodes 955  99  78  78 "$OutDir\node_mutate.png"
Key-And-Save $nodes 1076 99  74  74 "$OutDir\node_output.png"
Key-And-Save $nodes 289 430  84  84 "$OutDir\node_inhibitor.png"   # NOT gate
Key-And-Save $nodes 497 430  84  84 "$OutDir\node_oscillator.png"  # delay clock
Key-And-Save $nodes 749 231  96  96 "$OutDir\badge_locked.png"
Key-And-Save $nodes 430 231 108 108 "$OutDir\badge_unstable.png"
Key-And-Save $nodes 597 231 108 108 "$OutDir\badge_damaged.png"

# --- logo plaque (sheet 1 top-left) ---
Key-And-Save $uiKit 210 80 372 122 "$OutDir\logo_plaque.png" 40 110

$uiKit.Dispose(); $nodes.Dispose()
Write-Host "done."
