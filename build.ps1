param (
    [switch]$debug
)

$config = "Release"
if ($debug) {
    $config = "Debug"
    Write-Host "Building in Debug mode..." -ForegroundColor Cyan
} else {
    Write-Host "Building in Release mode..." -ForegroundColor Cyan
}

cmake -B build -S .
cmake --build build --config $config
