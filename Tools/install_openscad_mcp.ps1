# OpenSCAD MCP Server kurulumu (Windows)
# Yapar: OpenSCAD + Python 3.12 (eksikse winget ile), repoyu indirir, venv kurar,
# test eder ve Claude Desktop ayarlarina "openscad" sunucusunu ekler (yedek alarak).
$ErrorActionPreference = "Stop"
$Base    = Join-Path $env:USERPROFILE "OpenSCAD-MCP"
$RepoDir = Join-Path $Base "OpenSCAD-MCP-Server"
$Models  = Join-Path $Base "models"
New-Item -ItemType Directory -Force -Path $Base, $Models | Out-Null

function Refresh-Path {
  $env:Path = [Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [Environment]::GetEnvironmentVariable("Path","User")
}
function Step($m) { Write-Host "`n==> $m" -ForegroundColor Cyan }

# 1) OpenSCAD
Step "OpenSCAD kontrol ediliyor"
$scad = @("$env:ProgramFiles\OpenSCAD\openscad.exe", "${env:ProgramFiles(x86)}\OpenSCAD\openscad.exe",
          "$env:ProgramFiles\OpenSCAD (Nightly)\openscad.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $scad) {
  Write-Host "OpenSCAD yok, winget ile kuruluyor..."
  winget install --id OpenSCAD.OpenSCAD -e --accept-source-agreements --accept-package-agreements
  $scad = @("$env:ProgramFiles\OpenSCAD\openscad.exe", "${env:ProgramFiles(x86)}\OpenSCAD\openscad.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $scad) { throw "OpenSCAD bulunamadi. https://openscad.org/downloads.html adresinden kurup tekrar calistir." }
Write-Host "OpenSCAD: $scad"

# 2) Python 3.11+
Step "Python 3.11+ kontrol ediliyor"
function Find-Python {
  foreach ($line in @("py -3.13","py -3.12","py -3.11","python")) {
    $c = $line.Split(" ")
    $rest = @(); if ($c.Length -gt 1) { $rest = $c[1..($c.Length-1)] }
    try {
      $exe = & $c[0] @rest -c "import sys; print(sys.executable if sys.version_info>=(3,11) else '')" 2>$null
      if ($LASTEXITCODE -eq 0 -and $exe -and (Test-Path $exe.Trim())) { return $exe.Trim() }
    } catch {}
  }
  return $null
}
$py = Find-Python
if (-not $py) {
  Write-Host "Uygun Python yok, winget ile Python 3.12 kuruluyor..."
  winget install --id Python.Python.3.12 -e --accept-source-agreements --accept-package-agreements
  Refresh-Path
  $py = Find-Python
  if (-not $py) {
    $cand = Join-Path $env:LOCALAPPDATA "Programs\Python\Python312\python.exe"
    if (Test-Path $cand) { $py = $cand }
  }
}
if (-not $py) { throw "Python 3.11+ bulunamadi." }
Write-Host "Python: $py"

# 3) Repo indir (git gerekmez)
Step "OpenSCAD-MCP-Server indiriliyor"
$zip = Join-Path $Base "repo.zip"
Invoke-WebRequest -UseBasicParsing "https://github.com/jhacksman/OpenSCAD-MCP-Server/archive/refs/heads/main.zip" -OutFile $zip
if (Test-Path $RepoDir) { Remove-Item -Recurse -Force $RepoDir }
Expand-Archive -Force $zip $Base
Move-Item (Join-Path $Base "OpenSCAD-MCP-Server-main") $RepoDir
Remove-Item $zip

# 4) venv + kurulum
Step "Sanal ortam ve paketler kuruluyor"
$venv = Join-Path $RepoDir "venv"
& $py -m venv $venv
$vpy = Join-Path $venv "Scripts\python.exe"
& $vpy -m pip install --upgrade pip
& $vpy -m pip install $RepoDir
$server = Join-Path $venv "Scripts\openscad-mcp.exe"
if (-not (Test-Path $server)) { throw "openscad-mcp.exe olusmadi." }

# 5) Test
Step "Test"
& $server --help | Select-Object -First 3
& $scad --version

# 6) Claude Desktop ayarlari
Step "Claude Desktop ayarlarina ekleniyor"
$cfgs = @()
$cfgs += Join-Path $env:APPDATA "Claude\claude_desktop_config.json"
Get-ChildItem (Join-Path $env:LOCALAPPDATA "Packages") -Directory -Filter "Claude_*" -ErrorAction SilentlyContinue | ForEach-Object {
  $p = Join-Path $_.FullName "LocalCache\Roaming\Claude\claude_desktop_config.json"
  if (Test-Path (Split-Path $p)) { $cfgs += $p }
}
$entry = @{ command = $server; args = @("--output-dir", $Models, "--openscad", $scad) }
$wrote = $false
foreach ($cfg in $cfgs) {
  if (-not (Test-Path (Split-Path $cfg))) { continue }
  $entryFile = Join-Path $Base "entry.json"
  $mergeFile = Join-Path $Base "merge_cfg.py"
  ($entry | ConvertTo-Json -Compress) | Set-Content -Encoding ASCII $entryFile
  @'
import json, os, shutil, sys
p = sys.argv[1]
entry = json.load(open(sys.argv[2], encoding='utf-8-sig'))
d = {}
if os.path.exists(p):
    shutil.copy2(p, p + '.backup')
    with open(p, encoding='utf-8-sig') as f:
        txt = f.read().strip()
        d = json.loads(txt) if txt else {}
d.setdefault('mcpServers', {})['openscad'] = entry
with open(p, 'w', encoding='utf-8') as f:
    json.dump(d, f, indent=2)
print('Yazildi:', p)
'@ | Set-Content -Encoding ASCII $mergeFile
  & $vpy $mergeFile $cfg $entryFile
  $wrote = $true
}
if (-not $wrote) {
  Write-Host "Claude ayar klasoru bulunamadi. Claude > Settings > Developer > Edit Config ile su girdiyi ekle:" -ForegroundColor Yellow
  @{ mcpServers = @{ openscad = $entry } } | ConvertTo-Json -Depth 5
}

Write-Host "`nBITTI. Claude masaustu uygulamasini tamamen kapatip (sistem tepsisinden de) yeniden ac." -ForegroundColor Green
Write-Host "Modeller burada olusacak: $Models"
