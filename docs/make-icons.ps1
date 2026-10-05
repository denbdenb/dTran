# Generates the placeholder app icons in /assets. Run once; output is committed.
# Uses System.Drawing only as a one-off dev tool - it is NOT part of the app.
Add-Type -AssemblyName System.Drawing

$out = Join-Path $PSScriptRoot '..\assets'
New-Item -ItemType Directory -Force -Path $out | Out-Null

function New-RoundedPath([float]$x, [float]$y, [float]$w, [float]$h, [float]$r) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = $r * 2
    $p.AddArc($x, $y, $d, $d, 180, 90)
    $p.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $p.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
    $p.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $p.CloseFigure()
    return $p
}

function New-Icon([int]$w, [int]$h, [string]$name, [double]$glyphScale = 0.55) {
    $bmp = New-Object System.Drawing.Bitmap($w, $h, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = 'AntiAlias'
    $g.TextRenderingHint = 'AntiAliasGridFit'
    $g.Clear([System.Drawing.Color]::Transparent)

    $side = [math]::Min($w, $h) * 0.82
    $x = ($w - $side) / 2
    $y = ($h - $side) / 2
    $path = New-RoundedPath $x $y $side $side ($side * 0.22)
    $p1 = [System.Drawing.PointF]::new([float]$x, [float]$y)
    $p2 = [System.Drawing.PointF]::new([float]$x, [float]($y + $side))
    $brush = [System.Drawing.Drawing2D.LinearGradientBrush]::new($p1, $p2,
        [System.Drawing.Color]::FromArgb(255, 74, 134, 240), [System.Drawing.Color]::FromArgb(255, 47, 105, 222))
    $g.FillPath($brush, $path)

    $font = New-Object System.Drawing.Font('Segoe UI Semibold', [float]($side * $glyphScale), [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $fmt = New-Object System.Drawing.StringFormat
    $fmt.Alignment = 'Center'; $fmt.LineAlignment = 'Center'
    $rect = New-Object System.Drawing.RectangleF($x, $y, $side, $side)
    $g.DrawString('dT', $font, [System.Drawing.Brushes]::White, $rect, $fmt)

    $bmp.Save((Join-Path $out $name), [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
}

New-Icon 44  44  'Square44x44Logo.png' 0.46
New-Icon 50  50  'StoreLogo.png' 0.46
New-Icon 150 150 'Square150x150Logo.png' 0.46
New-Icon 310 150 'Wide310x150Logo.png' 0.46
New-Icon 620 300 'SplashScreen.png' 0.46
Get-ChildItem $out | Select-Object Name, Length
