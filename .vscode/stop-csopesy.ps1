$target = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\build\Debug\csopesy.exe'))

Get-CimInstance Win32_Process |
    Where-Object {
        $_.Name -eq 'csopesy.exe' -and
        $_.ExecutablePath -and
        ([System.IO.Path]::GetFullPath($_.ExecutablePath) -eq $target)
    } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force }