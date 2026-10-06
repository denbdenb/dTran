# Generates lightweight local SVG flags for dTranslate
param([string]$OutDir = "$PSScriptRoot\..\assets\flags")

if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
}

function Save-Svg($name, $content) {
    $svg = @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 18" width="24" height="18">
$content
</svg>
"@
    $path = Join-Path $OutDir "$name.svg"
    [System.IO.File]::WriteAllText($path, $svg, [System.Text.Encoding]::UTF8)
}

# 1. Globe (auto)
Save-Svg "auto" @'
  <rect width="24" height="18" rx="2" fill="#2563EB"/>
  <circle cx="12" cy="9" r="6.5" fill="none" stroke="#FFFFFF" stroke-width="1.2"/>
  <ellipse cx="12" cy="9" rx="3.2" ry="6.5" fill="none" stroke="#FFFFFF" stroke-width="1"/>
  <line x1="5.5" y1="9" x2="18.5" y2="9" stroke="#FFFFFF" stroke-width="1"/>
  <line x1="6.8" y1="5.5" x2="17.2" y2="5.5" stroke="#FFFFFF" stroke-width="0.8"/>
  <line x1="6.8" y1="12.5" x2="17.2" y2="12.5" stroke="#FFFFFF" stroke-width="0.8"/>
'@

# 2. Russian tricolor (ru)
Save-Svg "ru" @'
  <rect width="24" height="6" fill="#FFFFFF"/>
  <rect y="6" width="24" height="6" fill="#0039A6"/>
  <rect y="12" width="24" height="6" fill="#D52B1E"/>
'@

# 3. USA (en)
Save-Svg "en" @'
  <rect width="24" height="18" fill="#B22234"/>
  <path d="M0,2.77h24 M0,5.54h24 M0,8.31h24 M0,11.08h24 M0,13.85h24 M0,16.62h24" stroke="#FFFFFF" stroke-width="1.38"/>
  <rect width="10" height="9.7" fill="#3C3B6E"/>
  <circle cx="5" cy="4.8" r="1.8" fill="#FFFFFF"/>
'@

# 4. Germany (de)
Save-Svg "de" @'
  <rect width="24" height="6" fill="#000000"/>
  <rect y="6" width="24" height="6" fill="#DD0000"/>
  <rect y="12" width="24" height="6" fill="#FFCE00"/>
'@

# 5. France (fr)
Save-Svg "fr" @'
  <rect width="8" height="18" fill="#002654"/>
  <rect x="8" width="8" height="18" fill="#FFFFFF"/>
  <rect x="16" width="8" height="18" fill="#ED2939"/>
'@

# 6. Spain (es)
Save-Svg "es" @'
  <rect width="24" height="4.5" fill="#AA151B"/>
  <rect y="4.5" width="24" height="9" fill="#F1BF00"/>
  <rect y="13.5" width="24" height="4.5" fill="#AA151B"/>
'@

# 7. Italy (it)
Save-Svg "it" @'
  <rect width="8" height="18" fill="#008C45"/>
  <rect x="8" width="8" height="18" fill="#F4F4F4"/>
  <rect x="16" width="8" height="18" fill="#CD212A"/>
'@

# 8. Portugal (pt)
Save-Svg "pt" @'
  <rect width="9" height="18" fill="#046A38"/>
  <rect x="9" width="15" height="18" fill="#DA291C"/>
  <circle cx="9" cy="9" r="3.2" fill="#FFCE00"/>
  <circle cx="9" cy="9" r="1.8" fill="#FFFFFF"/>
'@

# 9. China (zh)
Save-Svg "zh" @'
  <rect width="24" height="18" fill="#EE1C25"/>
  <polygon points="4,2 4.6,3.8 6.5,3.8 5,5 5.5,6.8 4,5.6 2.5,6.8 3,5 1.5,3.8 3.4,3.8" fill="#FFDE00"/>
  <circle cx="8" cy="2.5" r="0.8" fill="#FFDE00"/>
  <circle cx="9.5" cy="4" r="0.8" fill="#FFDE00"/>
  <circle cx="9.5" cy="6" r="0.8" fill="#FFDE00"/>
  <circle cx="8" cy="7.5" r="0.8" fill="#FFDE00"/>
'@

# 10. Taiwan (zh-TW)
Save-Svg "zh-TW" @'
  <rect width="24" height="18" fill="#FE0000"/>
  <rect width="12" height="9" fill="#000095"/>
  <circle cx="6" cy="4.5" r="2.2" fill="#FFFFFF"/>
  <circle cx="6" cy="4.5" r="1.5" fill="#000095"/>
  <circle cx="6" cy="4.5" r="1.1" fill="#FFFFFF"/>
'@

# 11. Japan (ja)
Save-Svg "ja" @'
  <rect width="24" height="18" fill="#FFFFFF"/>
  <circle cx="12" cy="9" r="5.2" fill="#BC002D"/>
'@

# 12. South Korea (ko)
Save-Svg "ko" @'
  <rect width="24" height="18" fill="#FFFFFF"/>
  <circle cx="12" cy="9" r="4.2" fill="#CD2E3A"/>
  <path d="M12,9 A2.1,2.1 0 0,0 12,13.2 A4.2,4.2 0 0,0 12,4.8 A2.1,2.1 0 0,1 12,9 Z" fill="#0047A0"/>
'@

# 13. Saudi Arabia (ar)
Save-Svg "ar" @'
  <rect width="24" height="18" fill="#006C35"/>
  <rect x="5" y="7" width="14" height="2" fill="#FFFFFF" rx="1"/>
  <line x1="5" y1="12" x2="19" y2="12" stroke="#FFFFFF" stroke-width="1.2"/>
'@

# 14. Turkey (tr)
Save-Svg "tr" @'
  <rect width="24" height="18" fill="#E30A17"/>
  <circle cx="9" cy="9" r="4.5" fill="#FFFFFF"/>
  <circle cx="10.2" cy="9" r="3.6" fill="#E30A17"/>
  <polygon points="14,9 15.5,9.5 17,9 16,10.5 16.5,12 15,11 13.5,12 14,10.5" fill="#FFFFFF"/>
'@

# 15. Ukraine (uk)
Save-Svg "uk" @'
  <rect width="24" height="9" fill="#0057B7"/>
  <rect y="9" width="24" height="9" fill="#FFDD00"/>
'@

# 16. Poland (pl)
Save-Svg "pl" @'
  <rect width="24" height="9" fill="#FFFFFF"/>
  <rect y="9" width="24" height="9" fill="#DC143C"/>
'@

# 17. Netherlands (nl)
Save-Svg "nl" @'
  <rect width="24" height="6" fill="#AE1C28"/>
  <rect y="6" width="24" height="6" fill="#FFFFFF"/>
  <rect y="12" width="24" height="6" fill="#21468B"/>
'@

# 18. Sweden (sv)
Save-Svg "sv" @'
  <rect width="24" height="18" fill="#006AA7"/>
  <rect x="7" width="3.5" height="18" fill="#FECC00"/>
  <rect y="7.25" width="24" height="3.5" fill="#FECC00"/>
'@

# 19. Norway (no)
Save-Svg "no" @'
  <rect width="24" height="18" fill="#BA0C2F"/>
  <rect x="6" width="5" height="18" fill="#FFFFFF"/>
  <rect y="6.5" width="24" height="5" fill="#FFFFFF"/>
  <rect x="7.25" width="2.5" height="18" fill="#00205B"/>
  <rect y="7.75" width="24" height="2.5" fill="#00205B"/>
'@

# 20. Denmark (da)
Save-Svg "da" @'
  <rect width="24" height="18" fill="#C8102E"/>
  <rect x="7" width="3" height="18" fill="#FFFFFF"/>
  <rect y="7.5" width="24" height="3" fill="#FFFFFF"/>
'@

# 21. Finland (fi)
Save-Svg "fi" @'
  <rect width="24" height="18" fill="#FFFFFF"/>
  <rect x="6.5" width="4.5" height="18" fill="#002F6C"/>
  <rect y="6.75" width="24" height="4.5" fill="#002F6C"/>
'@

# 22. Greece (el)
Save-Svg "el" @'
  <rect width="24" height="18" fill="#0D5EAF"/>
  <path d="M0,2h24 M0,6h24 M0,10h24 M0,14h24" stroke="#FFFFFF" stroke-width="2"/>
  <rect width="10" height="10" fill="#0D5EAF"/>
  <rect x="4" width="2" height="10" fill="#FFFFFF"/>
  <rect y="4" width="10" height="2" fill="#FFFFFF"/>
'@

# 23. Czechia (cs)
Save-Svg "cs" @'
  <rect width="24" height="9" fill="#FFFFFF"/>
  <rect y="9" width="24" height="9" fill="#D7141A"/>
  <polygon points="0,0 12,9 0,18" fill="#11457E"/>
'@

# 24. Slovakia (sk)
Save-Svg "sk" @'
  <rect width="24" height="6" fill="#FFFFFF"/>
  <rect y="6" width="24" height="6" fill="#0B4EA2"/>
  <rect y="12" width="24" height="6" fill="#EE1C25"/>
  <rect x="4" y="4" width="5" height="8" rx="2" fill="#EE1C25" stroke="#FFFFFF" stroke-width="0.8"/>
'@

# 25. Hungary (hu)
Save-Svg "hu" @'
  <rect width="24" height="6" fill="#CE2939"/>
  <rect y="6" width="24" height="6" fill="#FFFFFF"/>
  <rect y="12" width="24" height="6" fill="#477050"/>
'@

# 26. Romania (ro)
Save-Svg "ro" @'
  <rect width="8" height="18" fill="#002B7F"/>
  <rect x="8" width="8" height="18" fill="#FCD116"/>
  <rect x="16" width="8" height="18" fill="#CE1126"/>
'@

# 27. Bulgaria (bg)
Save-Svg "bg" @'
  <rect width="24" height="6" fill="#FFFFFF"/>
  <rect y="6" width="24" height="6" fill="#00966E"/>
  <rect y="12" width="24" height="6" fill="#D62612"/>
'@

# 28. Serbia (sr)
Save-Svg "sr" @'
  <rect width="24" height="6" fill="#C6363C"/>
  <rect y="6" width="24" height="6" fill="#0C4076"/>
  <rect y="12" width="24" height="6" fill="#FFFFFF"/>
'@

# 29. Croatia (hr)
Save-Svg "hr" @'
  <rect width="24" height="6" fill="#FF0000"/>
  <rect y="6" width="24" height="6" fill="#FFFFFF"/>
  <rect y="12" width="24" height="6" fill="#0000FF"/>
  <rect x="9" y="5" width="6" height="7" fill="#FF0000" stroke="#FFFFFF" stroke-width="0.5"/>
'@

# 30. Slovenia (sl)
Save-Svg "sl" @'
  <rect width="24" height="6" fill="#FFFFFF"/>
  <rect y="6" width="24" height="6" fill="#002F6C"/>
  <rect y="12" width="24" height="6" fill="#ED1C24"/>
  <polygon points="5,4 7,8 3,8" fill="#002F6C"/>
'@

# 31. Israel (he)
Save-Svg "he" @'
  <rect width="24" height="18" fill="#FFFFFF"/>
  <rect y="2" width="24" height="2.5" fill="#0038B8"/>
  <rect y="13.5" width="24" height="2.5" fill="#0038B8"/>
  <polygon points="12,5.5 15,11 9,11" fill="none" stroke="#0038B8" stroke-width="0.8"/>
  <polygon points="12,12.5 15,7 9,7" fill="none" stroke="#0038B8" stroke-width="0.8"/>
'@

# 32. India (hi, ta, te, kn, ml, gu, mr, pa)
$indiaSvg = @'
  <rect width="24" height="6" fill="#FF9933"/>
  <rect y="6" width="24" height="6" fill="#FFFFFF"/>
  <rect y="12" width="24" height="6" fill="#138808"/>
  <circle cx="12" cy="9" r="2.2" fill="none" stroke="#000080" stroke-width="0.7"/>
'@
Save-Svg "hi" $indiaSvg
Save-Svg "ta" $indiaSvg
Save-Svg "te" $indiaSvg
Save-Svg "kn" $indiaSvg
Save-Svg "ml" $indiaSvg
Save-Svg "gu" $indiaSvg
Save-Svg "mr" $indiaSvg
Save-Svg "pa" $indiaSvg

# 33. Bangladesh (bn)
Save-Svg "bn" @'
  <rect width="24" height="18" fill="#006A4E"/>
  <circle cx="10.5" cy="9" r="5" fill="#F42A41"/>
'@

# 34. Thailand (th)
Save-Svg "th" @'
  <rect width="24" height="3" fill="#A51931"/>
  <rect y="3" width="24" height="3" fill="#F4F5F8"/>
  <rect y="6" width="24" height="6" fill="#2D2A4A"/>
  <rect y="12" width="24" height="3" fill="#F4F5F8"/>
  <rect y="15" width="24" height="3" fill="#A51931"/>
'@

# 35. Vietnam (vi)
Save-Svg "vi" @'
  <rect width="24" height="18" fill="#DA251D"/>
  <polygon points="12,4 13.5,8.5 18,8.5 14.5,11.2 16,15.5 12,13 8,15.5 9.5,11.2 6,8.5 10.5,8.5" fill="#FFFF00"/>
'@

# 36. Indonesia (id, jv, su)
$indoSvg = @'
  <rect width="24" height="9" fill="#FF0000"/>
  <rect y="9" width="24" height="9" fill="#FFFFFF"/>
'@
Save-Svg "id" $indoSvg
Save-Svg "jv" $indoSvg
Save-Svg "su" $indoSvg

# 37. Malaysia (ms)
Save-Svg "ms" @'
  <rect width="24" height="18" fill="#CC0000"/>
  <path d="M0,2.5h24 M0,5h24 M0,7.5h24 M0,10h24 M0,12.5h24 M0,15h24 M0,17.5h24" stroke="#FFFFFF" stroke-width="1.25"/>
  <rect width="12" height="9.5" fill="#000066"/>
  <circle cx="6" cy="4.75" r="2.5" fill="#FFCC00"/>
  <circle cx="7" cy="4.75" r="2.1" fill="#000066"/>
'@

# 38. Philippines (fil)
Save-Svg "fil" @'
  <rect width="24" height="9" fill="#0038A8"/>
  <rect y="9" width="24" height="9" fill="#CE1126"/>
  <polygon points="0,0 12,9 0,18" fill="#FFFFFF"/>
  <circle cx="4" cy="9" r="1.8" fill="#FCD116"/>
'@

# 39. Iran (fa)
Save-Svg "fa" @'
  <rect width="24" height="6" fill="#239F40"/>
  <rect y="6" width="24" height="6" fill="#FFFFFF"/>
  <rect y="12" width="24" height="6" fill="#DA0000"/>
  <circle cx="12" cy="9" r="1.8" fill="#DA0000"/>
'@

# 40. Pakistan (ur)
Save-Svg "ur" @'
  <rect width="6" height="18" fill="#FFFFFF"/>
  <rect x="6" width="18" height="18" fill="#01411C"/>
  <circle cx="15" cy="9" r="4.2" fill="#FFFFFF"/>
  <circle cx="16.2" cy="8.2" r="3.6" fill="#01411C"/>
  <circle cx="16.8" cy="7" r="0.8" fill="#FFFFFF"/>
'@

# 41. Nepal (ne)
Save-Svg "ne" @'
  <polygon points="2,1 18,10 8,10 18,17 2,17" fill="#DC143C" stroke="#003893" stroke-width="1.5"/>
  <circle cx="6" cy="6" r="1.5" fill="#FFFFFF"/>
  <circle cx="6" cy="13.5" r="1.5" fill="#FFFFFF"/>
'@

# 42. Sri Lanka (si)
Save-Svg "si" @'
  <rect width="4" height="18" fill="#005A36"/>
  <rect x="4" width="4" height="18" fill="#FF7900"/>
  <rect x="8" width="16" height="18" fill="#8D153A"/>
  <circle cx="16" cy="9" r="3" fill="#FFBE29"/>
'@

# 43. Myanmar (my)
Save-Svg "my" @'
  <rect width="24" height="6" fill="#FECB00"/>
  <rect y="6" width="24" height="6" fill="#34B233"/>
  <rect y="12" width="24" height="6" fill="#EA2839"/>
  <polygon points="12,5 13.5,8.5 17,8.5 14,10.8 15.2,14.5 12,12.2 8.8,14.5 10,10.8 7,8.5 10.5,8.5" fill="#FFFFFF"/>
'@

# 44. Cambodia (km)
Save-Svg "km" @'
  <rect width="24" height="4.5" fill="#032EA6"/>
  <rect y="4.5" width="24" height="9" fill="#ED1B24"/>
  <rect y="13.5" width="24" height="4.5" fill="#032EA6"/>
  <rect x="9.5" y="6.5" width="5" height="5" fill="#FFFFFF" rx="1"/>
'@

# 45. Laos (lo)
Save-Svg "lo" @'
  <rect width="24" height="4.5" fill="#CE1126"/>
  <rect y="4.5" width="24" height="9" fill="#002868"/>
  <rect y="13.5" width="24" height="4.5" fill="#CE1126"/>
  <circle cx="12" cy="9" r="3.2" fill="#FFFFFF"/>
'@

# 46. Mongolia (mn)
Save-Svg "mn" @'
  <rect width="8" height="18" fill="#E4002B"/>
  <rect x="8" width="8" height="18" fill="#0066B3"/>
  <rect x="16" width="8" height="18" fill="#E4002B"/>
  <circle cx="4" cy="9" r="2.2" fill="#FFD100"/>
'@

# 47. Georgia (ka)
Save-Svg "ka" @'
  <rect width="24" height="18" fill="#FFFFFF"/>
  <rect x="10" width="4" height="18" fill="#FF0000"/>
  <rect y="7" width="24" height="4" fill="#FF0000"/>
  <rect x="4" y="3" width="2" height="2" fill="#FF0000"/>
  <rect x="18" y="3" width="2" height="2" fill="#FF0000"/>
  <rect x="4" y="13" width="2" height="2" fill="#FF0000"/>
  <rect x="18" y="13" width="2" height="2" fill="#FF0000"/>
'@

# 48. Armenia (hy)
Save-Svg "hy" @'
  <rect width="24" height="6" fill="#D90012"/>
  <rect y="6" width="24" height="6" fill="#0033A0"/>
  <rect y="12" width="24" height="6" fill="#F2A800"/>
'@

# 49. Azerbaijan (az)
Save-Svg "az" @'
  <rect width="24" height="6" fill="#00B5E2"/>
  <rect y="6" width="24" height="6" fill="#EF3340"/>
  <rect y="12" width="24" height="6" fill="#509E2F"/>
  <circle cx="12" cy="9" r="2" fill="#FFFFFF"/>
  <circle cx="12.5" cy="9" r="1.6" fill="#EF3340"/>
'@

# 50. Kazakhstan (kk)
Save-Svg "kk" @'
  <rect width="24" height="18" fill="#00AFCA"/>
  <circle cx="12" cy="8" r="2.8" fill="#FEC50C"/>
  <ellipse cx="12" cy="12.5" rx="4" ry="1.2" fill="#FEC50C"/>
'@

# 51. Uzbekistan (uz)
Save-Svg "uz" @'
  <rect width="24" height="5.5" fill="#0099B5"/>
  <rect y="5.5" width="24" height="1" fill="#CE1126"/>
  <rect y="6.5" width="24" height="5" fill="#FFFFFF"/>
  <rect y="11.5" width="24" height="1" fill="#CE1126"/>
  <rect y="12.5" width="24" height="5.5" fill="#1EB53A"/>
  <circle cx="4" cy="3" r="1.5" fill="#FFFFFF"/>
'@

# 52. Tajikistan (tg)
Save-Svg "tg" @'
  <rect width="24" height="5" fill="#CC0000"/>
  <rect y="5" width="24" height="8" fill="#FFFFFF"/>
  <rect y="13" width="24" height="5" fill="#006600"/>
  <circle cx="12" cy="9" r="1.8" fill="#F8B800"/>
'@

# 53. Kyrgyzstan (ky)
Save-Svg "ky" @'
  <rect width="24" height="18" fill="#E4002B"/>
  <circle cx="12" cy="9" r="4.2" fill="#FFE600"/>
  <circle cx="12" cy="9" r="3.2" fill="#E4002B"/>
'@

# 54. Turkmenistan (tk)
Save-Svg "tk" @'
  <rect width="24" height="18" fill="#009A44"/>
  <rect x="3" width="4" height="18" fill="#D3202A"/>
  <circle cx="11" cy="4.5" r="2" fill="#FFFFFF"/>
'@

# 55. Belarus (be)
Save-Svg "be" @'
  <rect width="4" height="18" fill="#FFFFFF"/>
  <line x1="2" y1="0" x2="2" y2="18" stroke="#C8102E" stroke-dasharray="1.5,1.5"/>
  <rect x="4" width="20" height="12" fill="#C8102E"/>
  <rect x="4" y="12" width="20" height="6" fill="#009639"/>
'@

# 56. Lithuania (lt)
Save-Svg "lt" @'
  <rect width="24" height="6" fill="#FDB913"/>
  <rect y="6" width="24" height="6" fill="#006A44"/>
  <rect y="12" width="24" height="6" fill="#C1272D"/>
'@

# 57. Latvia (lv)
Save-Svg "lv" @'
  <rect width="24" height="7" fill="#9E3039"/>
  <rect y="7" width="24" height="4" fill="#FFFFFF"/>
  <rect y="11" width="24" height="7" fill="#9E3039"/>
'@

# 58. Estonia (et)
Save-Svg "et" @'
  <rect width="24" height="6" fill="#0072CE"/>
  <rect y="6" width="24" height="6" fill="#000000"/>
  <rect y="12" width="24" height="6" fill="#FFFFFF"/>
'@

# 59. Albania (sq)
Save-Svg "sq" @'
  <rect width="24" height="18" fill="#DA291C"/>
  <polygon points="12,5 14,8 16,7 14.5,10 16,13 13,11 12,14 11,11 8,13 9.5,10 8,7 10,8" fill="#000000"/>
'@

# 60. North Macedonia (mk)
Save-Svg "mk" @'
  <rect width="24" height="18" fill="#D20000"/>
  <polygon points="0,0 24,18 24,0 0,18" stroke="#FFE600" stroke-width="2"/>
  <line x1="12" y1="0" x2="12" y2="18" stroke="#FFE600" stroke-width="2.5"/>
  <line x1="0" y1="9" x2="24" y2="9" stroke="#FFE600" stroke-width="2.5"/>
  <circle cx="12" cy="9" r="3.2" fill="#FFE600" stroke="#D20000" stroke-width="0.8"/>
'@

# 61. Bosnia and Herzegovina (bs)
Save-Svg "bs" @'
  <rect width="24" height="18" fill="#002395"/>
  <polygon points="7,0 19,0 19,18" fill="#FECB00"/>
  <circle cx="9" cy="4" r="0.7" fill="#FFFFFF"/>
  <circle cx="11" cy="7" r="0.7" fill="#FFFFFF"/>
  <circle cx="13" cy="10" r="0.7" fill="#FFFFFF"/>
  <circle cx="15" cy="13" r="0.7" fill="#FFFFFF"/>
'@

# 62. Iceland (is)
Save-Svg "is" @'
  <rect width="24" height="18" fill="#02529C"/>
  <rect x="7" width="4" height="18" fill="#FFFFFF"/>
  <rect y="7" width="24" height="4" fill="#FFFFFF"/>
  <rect x="8" width="2" height="18" fill="#DC1E35"/>
  <rect y="8" width="24" height="2" fill="#DC1E35"/>
'@

# 63. Ireland (ga)
Save-Svg "ga" @'
  <rect width="8" height="18" fill="#169B62"/>
  <rect x="8" width="8" height="18" fill="#FFFFFF"/>
  <rect x="16" width="8" height="18" fill="#FF883E"/>
'@

# 64. Wales (cy)
Save-Svg "cy" @'
  <rect width="24" height="9" fill="#FFFFFF"/>
  <rect y="9" width="24" height="9" fill="#00AB39"/>
  <polygon points="8,7 16,9 12,13" fill="#D30731"/>
'@

# 65. European / Regional (eu)
Save-Svg "eu" @'
  <rect width="24" height="18" fill="#003399"/>
  <circle cx="12" cy="9" r="5" fill="none" stroke="#FFCC00" stroke-width="1.2" stroke-dasharray="1.2,1.4"/>
'@

# 66. Catalonia (ca)
Save-Svg "ca" @'
  <rect width="24" height="18" fill="#FCD116"/>
  <path d="M0,2h24 M0,6h24 M0,10h24 M0,14h24" stroke="#CE1126" stroke-width="2"/>
'@

# 67. Galicia (gl)
Save-Svg "gl" @'
  <rect width="24" height="18" fill="#FFFFFF"/>
  <polygon points="0,0 4,0 24,18 20,18" fill="#007AC2"/>
'@

# 68. Malta (mt)
Save-Svg "mt" @'
  <rect width="12" height="18" fill="#FFFFFF"/>
  <rect x="12" width="12" height="18" fill="#CF142B"/>
  <rect x="2" y="2" width="3" height="3" fill="#808080"/>
'@

# 69. Esperanto (eo)
Save-Svg "eo" @'
  <rect width="24" height="18" fill="#009900"/>
  <rect width="9" height="7" fill="#FFFFFF"/>
  <polygon points="4.5,1.5 5.3,3.5 7.2,3.5 5.7,4.8 6.2,6.7 4.5,5.5 2.8,6.7 3.3,4.8 1.8,3.5 3.7,3.5" fill="#009900"/>
'@

# 70. Latin / Vatican (la)
Save-Svg "la" @'
  <rect width="12" height="18" fill="#FFE000"/>
  <rect x="12" width="12" height="18" fill="#FFFFFF"/>
  <circle cx="18" cy="9" r="2.5" fill="#C0C0C0"/>
'@

# 71. South Africa (af, zu)
$zaSvg = @'
  <rect width="24" height="9" fill="#E03C31"/>
  <rect y="9" width="24" height="9" fill="#001489"/>
  <polygon points="0,0 10,9 0,18" fill="#000000"/>
  <polygon points="0,0 12,9 0,18" fill="none" stroke="#FFB81C" stroke-width="1.5"/>
  <polygon points="0,2 9,9 0,16" fill="none" stroke="#007749" stroke-width="3"/>
'@
Save-Svg "af" $zaSvg
Save-Svg "zu" $zaSvg

# 72. Swahili (sw)
Save-Svg "sw" @'
  <rect width="24" height="18" fill="#1EB53A"/>
  <polygon points="0,18 24,0 24,18" fill="#00A3DD"/>
  <polygon points="0,18 0,15 20,0 24,0 24,3 4,18" fill="#000000" stroke="#FCD116" stroke-width="1"/>
'@

# 73. Ethiopia (am)
Save-Svg "am" @'
  <rect width="24" height="6" fill="#078930"/>
  <rect y="6" width="24" height="6" fill="#FCDD09"/>
  <rect y="12" width="24" height="6" fill="#DA121A"/>
  <circle cx="12" cy="9" r="2.5" fill="#0F47AF"/>
'@

# 74. Somalia (so)
Save-Svg "so" @'
  <rect width="24" height="18" fill="#4189DD"/>
  <polygon points="12,4.5 13.5,8.5 17.5,8.5 14.2,11 15.5,15 12,12.5 8.5,15 9.8,11 6.5,8.5 10.5,8.5" fill="#FFFFFF"/>
'@

# 75. Nigeria (yo, ig, ha)
$ngSvg = @'
  <rect width="8" height="18" fill="#008751"/>
  <rect x="8" width="8" height="18" fill="#FFFFFF"/>
  <rect x="16" width="8" height="18" fill="#008751"/>
'@
Save-Svg "yo" $ngSvg
Save-Svg "ig" $ngSvg
Save-Svg "ha" $ngSvg

# 76. Madagascar (mg)
Save-Svg "mg" @'
  <rect width="8" height="18" fill="#FFFFFF"/>
  <rect x="8" width="16" height="9" fill="#FC3D32"/>
  <rect x="8" y="9" width="16" height="9" fill="#007E3A"/>
'@

# 77. Kurdish (ku)
Save-Svg "ku" @'
  <rect width="24" height="6" fill="#E41B13"/>
  <rect y="6" width="24" height="6" fill="#FFFFFF"/>
  <rect y="12" width="24" height="6" fill="#138808"/>
  <circle cx="12" cy="9" r="2.5" fill="#FFC800"/>
'@

# 78. Pashto (ps)
Save-Svg "ps" @'
  <rect width="8" height="18" fill="#000000"/>
  <rect x="8" width="8" height="18" fill="#D32011"/>
  <rect x="16" width="8" height="18" fill="#007A3D"/>
  <circle cx="12" cy="9" r="2.2" fill="#FFFFFF"/>
'@

# 79. Yiddish (yi)
Save-Svg "yi" @'
  <rect width="24" height="18" fill="#FFFFFF"/>
  <rect y="2" width="24" height="2" fill="#0038B8"/>
  <rect y="14" width="24" height="2" fill="#0038B8"/>
  <circle cx="12" cy="9" r="2.8" fill="#0038B8"/>
'@

# 80. Luxembourg (lb)
Save-Svg "lb" @'
  <rect width="24" height="6" fill="#EA141D"/>
  <rect y="6" width="24" height="6" fill="#FFFFFF"/>
  <rect y="12" width="24" height="6" fill="#00A1DE"/>
'@

# 81. Maori (mi)
Save-Svg "mi" @'
  <rect width="24" height="9" fill="#000000"/>
  <rect y="9" width="24" height="9" fill="#CC0000"/>
  <circle cx="8" cy="9" r="3.2" fill="#FFFFFF"/>
  <circle cx="8" cy="9" r="1.8" fill="#000000"/>
'@

# 82. Samoa (sm)
Save-Svg "sm" @'
  <rect width="24" height="18" fill="#CE1126"/>
  <rect width="12" height="9" fill="#002B7F"/>
  <circle cx="6" cy="4.5" r="1.5" fill="#FFFFFF"/>
'@

# 83. Hawaii (haw)
Save-Svg "haw" @'
  <rect width="24" height="18" fill="#FFFFFF"/>
  <path d="M0,2.25h24 M0,6.75h24 M0,11.25h24 M0,15.75h24" stroke="#C8102E" stroke-width="2.25"/>
  <path d="M0,4.5h24 M0,9h24 M0,13.5h24" stroke="#00205B" stroke-width="2.25"/>
  <rect width="10" height="9" fill="#00205B"/>
  <line x1="0" y1="0" x2="10" y2="9" stroke="#FFFFFF" stroke-width="1.5"/>
  <line x1="10" y1="0" x2="0" y2="9" stroke="#FFFFFF" stroke-width="1.5"/>
  <rect x="4" width="2" height="9" fill="#C8102E"/>
  <rect y="3.5" width="10" height="2" fill="#C8102E"/>
'@

Write-Host "Flags generated successfully." -ForegroundColor Green
