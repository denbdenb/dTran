Add-Type -AssemblyName System.Drawing

$sizes = @(16, 24, 32, 48, 64, 128, 256)
$outPath = Join-Path $PSScriptRoot "..\assets\app.ico"
$pngStreams = @()

foreach ($sz in $sizes) {
    $bmp = New-Object System.Drawing.Bitmap $sz, $sz, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit

    $g.Clear([System.Drawing.Color]::Transparent)

    # Rounded rectangle background
    $pad = [Math]::Max(1.0, [double]$sz * 0.04)
    $w = [double]$sz - 2.0 * $pad
    $h = [double]$sz - 2.0 * $pad
    $radius = [Math]::Max(2.0, [double]$sz * 0.22)

    $rect = New-Object System.Drawing.RectangleF $pad, $pad, $w, $h
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $diameter = $radius * 2.0

    $path.AddArc($rect.X, $rect.Y, $diameter, $diameter, 180.0, 90.0)
    $path.AddArc($rect.Right - $diameter, $rect.Y, $diameter, $diameter, 270.0, 90.0)
    $path.AddArc($rect.Right - $diameter, $rect.Bottom - $diameter, $diameter, $diameter, 0.0, 90.0)
    $path.AddArc($rect.X, $rect.Bottom - $diameter, $diameter, $diameter, 90.0, 90.0)
    $path.CloseFigure()

    # Gradient brush: Fluent Blue
    $gradBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        (New-Object System.Drawing.PointF 0, 0),
        (New-Object System.Drawing.PointF 0, $sz),
        ([System.Drawing.Color]::FromArgb(255, 30, 144, 255)),   # Dodger Blue
        ([System.Drawing.Color]::FromArgb(255, 0, 90, 200))      # Deep Fluent Blue
    )
    $g.FillPath($gradBrush, $path)

    # Subtle inner border for contrast
    if ($sz -ge 32) {
        $pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(80, 255, 255, 255)), 1.0
        $g.DrawPath($pen, $path)
        $pen.Dispose()
    }

    # Text "dT"
    $fontSize = [float]($sz * 0.52)
    if ($sz -le 16) { $fontSize = 8.5 }
    elseif ($sz -le 24) { $fontSize = 12.0 }

    $font = New-Object System.Drawing.Font "Segoe UI", $fontSize, ([System.Drawing.FontStyle]::Bold)
    $sf = New-Object System.Drawing.StringFormat
    $sf.Alignment = [System.Drawing.StringAlignment]::Center
    $sf.LineAlignment = [System.Drawing.StringAlignment]::Center

    # Text drop shadow at larger sizes
    if ($sz -ge 48) {
        $shadowBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(70, 0, 20, 60))
        $shadowRect = New-Object System.Drawing.RectangleF ($pad + 1.0), ($pad + 1.5), $w, $h
        $g.DrawString("dT", $font, $shadowBrush, $shadowRect, $sf)
        $shadowBrush.Dispose()
    }

    $whiteBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::White)
    $g.DrawString("dT", $font, $whiteBrush, $rect, $sf)

    $whiteBrush.Dispose()
    $sf.Dispose()
    $font.Dispose()
    $gradBrush.Dispose()
    $path.Dispose()
    $g.Dispose()

    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    $pngStreams += @{ Size = $sz; Bytes = $ms.ToArray() }
    $ms.Dispose()
}

# Write multi-resolution ICO file
$fs = [System.IO.File]::Create($outPath)
$bw = New-Object System.IO.BinaryWriter $fs

# ICONDIR
$bw.Write([uint16]0) # Reserved
$bw.Write([uint16]1) # Type: 1 = ICO
$bw.Write([uint16]$pngStreams.Count) # Count

# Header size is 6 bytes + 16 bytes per entry
$dataOffset = 6 + (16 * $pngStreams.Count)

foreach ($item in $pngStreams) {
    $s = $item.Size
    $bWidth = if ($s -ge 256) { [byte]0 } else { [byte]$s }
    $bHeight = if ($s -ge 256) { [byte]0 } else { [byte]$s }
    
    $bw.Write($bWidth)
    $bw.Write($bHeight)
    $bw.Write([byte]0) # Colors
    $bw.Write([byte]0) # Reserved
    $bw.Write([uint16]1) # Color planes
    $bw.Write([uint16]32) # Bits per pixel
    $bw.Write([uint32]$item.Bytes.Length)
    $bw.Write([uint32]$dataOffset)

    $dataOffset += $item.Bytes.Length
}

foreach ($item in $pngStreams) {
    $bw.Write($item.Bytes)
}

$bw.Close()
$fs.Close()
Write-Output "Successfully generated $outPath with $($pngStreams.Count) icon frames."
