# Build Application.ico from the hex-brain glyph on the UI kit sheet.
Add-Type -AssemblyName System.Drawing

$sheet = New-Object System.Drawing.Bitmap("D:\UEprojects\BrainForge\images\ChatGPT Image Jul 10, 2026, 12_04_13 PM (1).png")
$key = $sheet.GetPixel(2, 2)

# crop glyph area (hex brain icon inside the logo plaque)
$cx = 88; $cy = 80; $size = 100
$x0 = $cx - $size / 2; $y0 = $cy - $size / 2

function New-KeyedCrop([System.Drawing.Bitmap]$src, [int]$x0, [int]$y0, [int]$size, [System.Drawing.Color]$key, [int]$outSize) {
    $crop = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    for ($y = 0; $y -lt $size; $y++) {
        for ($x = 0; $x -lt $size; $x++) {
            $p = $src.GetPixel($x0 + $x, $y0 + $y)
            $dist = [Math]::Sqrt(([double]$p.R - $key.R) * ($p.R - $key.R) + ([double]$p.G - $key.G) * ($p.G - $key.G) + ([double]$p.B - $key.B) * ($p.B - $key.B))
            $a = 0
            if ($dist -ge 150) { $a = 255 } elseif ($dist -gt 55) { $a = [int](($dist - 55) / 95 * 255) }
            $r = $p.R; $g = $p.G; $b = $p.B
            if ($r -gt $g -and $b -gt $g) {
                $ex = [Math]::Min($r, $b) - $g
                $r = [Math]::Max(0, $r - [int]($ex * 0.65)); $b = [Math]::Max(0, $b - [int]($ex * 0.35))
            }
            $crop.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($a, $r, $g, $b))
        }
    }
    # compose on dark disc background so the icon reads at small sizes
    $out = New-Object System.Drawing.Bitmap($outSize, $outSize, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g2 = [System.Drawing.Graphics]::FromImage($out)
    $g2.SmoothingMode = "AntiAlias"
    $g2.InterpolationMode = "HighQualityBicubic"
    $brush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 4, 10, 22))
    $g2.FillEllipse($brush, 0, 0, $outSize - 1, $outSize - 1)
    $pad = [int]($outSize * 0.10)
    $g2.DrawImage($crop, $pad, $pad, $outSize - 2 * $pad, $outSize - 2 * $pad)
    $g2.Dispose(); $crop.Dispose()
    return $out
}

# ICO with PNG-compressed entries
$sizes = @(256, 64, 48, 32, 16)
$pngs = @()
foreach ($s in $sizes) {
    $bmp = New-KeyedCrop $sheet $x0 $y0 $size $key $s
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $pngs += , $ms.ToArray()
    $bmp.Dispose()
}
$sheet.Dispose()

$outPath = "D:\UEprojects\BrainForge\Build\Windows\Application.ico"
New-Item -ItemType Directory -Force (Split-Path $outPath) | Out-Null
$fs = [System.IO.File]::Create($outPath)
$bw = New-Object System.IO.BinaryWriter($fs)
$bw.Write([UInt16]0); $bw.Write([UInt16]1); $bw.Write([UInt16]$sizes.Count)
$offset = 6 + 16 * $sizes.Count
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $s = $sizes[$i]
    $bw.Write([Byte]($(if ($s -eq 256) { 0 } else { $s })))  # width
    $bw.Write([Byte]($(if ($s -eq 256) { 0 } else { $s })))  # height
    $bw.Write([Byte]0); $bw.Write([Byte]0)
    $bw.Write([UInt16]1); $bw.Write([UInt16]32)
    $bw.Write([UInt32]$pngs[$i].Length)
    $bw.Write([UInt32]$offset)
    $offset += $pngs[$i].Length
}
foreach ($png in $pngs) { $bw.Write($png) }
$bw.Close(); $fs.Close()
Write-Host "wrote $outPath"
