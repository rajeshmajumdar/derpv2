# --- 1. SMART PATH DISCOVERY ---
$GccPath = Get-ChildItem -Path "C:\Qt" -Filter "gcc.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
if (!$GccPath) {
    Write-Host "CRITICAL ERROR: Could not find gcc.exe inside C:\Qt!" -ForegroundColor Red
    Write-Host "Please ensure 'MinGW' is installed via the Qt Maintenance Tool."
    exit
}
$MinGWBin = Split-Path $GccPath

$CMakeSearch = Get-ChildItem -Path "C:\Qt\Tools" -Filter "cmake.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
$CMakeBin = Split-Path $CMakeSearch

$WindeploySearch = Get-ChildItem -Path "C:\Qt" -Filter "windeployqt.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
if (!$WindeploySearch) {
    Write-Host "CRITICAL ERROR: Could not find windeployqt.exe inside C:\Qt!" -ForegroundColor Red
    exit
}
$QtBinDir = Split-Path $WindeploySearch
$QtPrefix = Split-Path $QtBinDir

Write-Host "Found Compiler at: $MinGWBin" -ForegroundColor Cyan
Write-Host "Found CMake at:    $CMakeBin" -ForegroundColor Cyan
Write-Host "Found Qt at:       $QtPrefix" -ForegroundColor Cyan

$C_COMP  = "$MinGWBin\gcc.exe"
$CXX_COMP = "$MinGWBin\g++.exe"

$env:PATH = "$MinGWBin;$CMakeBin;$QtBinDir;" + $env:PATH

if (!(Test-Path "build")) {
  Write-Host "Build folder not found. Please run 'run.ps1' first to generate CMake cache." -ForegroundColor Red
  exit
}

$watcher = New-Object System.IO.FileSystemWatcher
$watcher.Path = "$pwd\src\plugins"
$watcher.IncludeSubdirectories = $true
$watcher.EnableRaisingEvents = $true
$watcher.NotifyFilter = [System.IO.NotifyFilters]::LastWrite

Write-Host "----------------------------------------------" -ForegroundColor Green
Write-Host "  dERP hot reloader active" -ForegroundColor Green
Write-Host "  Found compiler at: $MinGWBin" -ForegroundColor Cyan
Write-Host "  Watching: src/plugins/ for changes..." -ForegroundColor DarkGray
Write-Host "  Press Ctrl+C to stop." -ForegroundColor DarkGray
Write-Host "----------------------------------------------" -ForegroundColor Green

$lastBuildTime = [DateTime]::MinValue

try {
  while ($true) {
    $result = $watcher.WaitForChanged([System.IO.WatcherChangeTypes]::Changed, 1000)

    if ($result.TimedOut -eq $false) {
      $fileName = $result.Name

      if ($fileName.EndsWith(".cpp") -or $fileName.EndsWith(".h") -or $fileName.EndsWith(".json")) {
        $now = [DateTime]::Now

        if (($now - $lastBuildTime).TotalSeconds -gt 1.5) {
          $lastBuildTime = $now

          Write-Host "`n[$(Get-Date -Format 'HH:mm:ss')] Change detected in: $fileName" -ForegroundColor Cyan
          Write-Host "Compiling..." -ForegroundColor DarkGray

          Push-Location .\build
          & mingw32-make -j8
          $exitCode = $LASTEXITCODE
          Pop-Location

          if ($exitCode -eq 0) {
            Write-Host "Build successful! Awaiting kernel hot-reloading..." -ForegroundColor Green
          } else {
            Write-Host "Build failed! Check compiler errors above." -ForegroundColor Red
          }
        }
      }
    }
  }
}

finally {
  $watcher.EnableRaisingEvents = $false
  $watcher.Dispose()
}

