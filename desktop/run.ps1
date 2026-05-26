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

# --- 2. BACKUP DATA FILES BEFORE CLEANUP ---
$backupDir = ".build_backup"
if (Test-Path $backupDir) { Remove-Item -Recurse -Force $backupDir }
New-Item -ItemType Directory -Path $backupDir | Out-Null

$filesToBackup = @("build\derp.db", "build\server.exe")
foreach ($f in $filesToBackup) {
    if (Test-Path $f) {
        Copy-Item $f $backupDir
        Write-Host "Backed up: $f" -ForegroundColor DarkGray
    }
}

# --- 3. CLEANUP ---
if (Test-Path build) { Remove-Item -Recurse -Force build }
New-Item -ItemType Directory -Path build
cd build

# --- 4. CONFIGURE ---
$cmakeArgs = @(
    "..",
    "-G", "MinGW Makefiles",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_PREFIX_PATH=$QtPrefix",
    "-DCMAKE_C_COMPILER=$C_COMP",
    "-DCMAKE_CXX_COMPILER=$CXX_COMP"
)

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { exit }

# --- 5. BUILD ---
& mingw32-make -j8
if ($LASTEXITCODE -ne 0) { exit }

# --- 6. DEPLOY QT DLLS ---
$exePath = "kernel.exe"
Write-Host "Deploying Qt dependencies..." -ForegroundColor Yellow
& "$QtBinDir\windeployqt.exe" --release --no-translations $exePath
if ($LASTEXITCODE -ne 0) {
    Write-Host "WARNING: windeployqt reported an issue (non-fatal)" -ForegroundColor Yellow
}

# --- 7. RESTORE BACKED-UP DATA FILES ---
$backupDirAbs = Join-Path (Split-Path (Get-Location)) $backupDir
foreach ($f in $filesToBackup) {
    $filename = Split-Path $f -Leaf
    $backedUpPath = Join-Path $backupDirAbs $filename
    if (Test-Path $backedUpPath) {
        Copy-Item $backedUpPath .
        Write-Host "Restored: $filename" -ForegroundColor Green
    }
}

# Clean up backup directory
if (Test-Path $backupDirAbs) { Remove-Item -Recurse -Force $backupDirAbs }

# --- 8. LAUNCH ---
Write-Host "Launching dERP..." -ForegroundColor Green
& "./$exePath"
