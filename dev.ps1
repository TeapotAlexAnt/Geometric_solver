$ErrorActionPreference = "Stop"

# Переходим в папку, где лежит сам скрипт
Set-Location $PSScriptRoot



$vitePath = "node_modules\.bin\vite.cmd"
$lockFile = "package-lock.json"
$installedLock = "node_modules\.package-lock.json"

# Проверяем, нет ли vite или изменился ли package-lock.json
$needInstall = -not (Test-Path $vitePath) -or `
               -not (Test-Path $installedLock) -or `
               ((Get-Item $lockFile).LastWriteTime -gt (Get-Item $installedLock).LastWriteTime)

if ($needInstall) {
    npm ci
}

# Запускаем dev-сервер и передаем все аргументы ($args)
npm run dev -- @args