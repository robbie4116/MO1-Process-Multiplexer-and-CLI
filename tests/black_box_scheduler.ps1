param(
    [Parameter(Mandatory = $true)]
    [string]$Executable
)

$ErrorActionPreference = 'Stop'

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )
    if (-not $Condition) {
        throw $Message
    }
}

function Invoke-SchedulerCase {
    param(
        [string]$Name,
        [int]$NumCpu,
        [string]$Scheduler,
        [uint32]$Quantum,
        [uint32]$BatchFrequency,
        [uint32]$Delay,
        [int]$WaitMilliseconds = 1200
    )

    $caseDirectory = Join-Path $env:TEMP ("csopesy-black-box-" + $Name)
    Remove-Item -LiteralPath $caseDirectory -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $caseDirectory | Out-Null
    Copy-Item -LiteralPath $Executable -Destination (Join-Path $caseDirectory 'csopesy.exe')

    @"
num-cpu $NumCpu
scheduler "$Scheduler"
quantum-cycles $Quantum
batch-process-freq $BatchFrequency
min-ins 1000
max-ins 1000
delay-per-exec $Delay
"@ | Set-Content -LiteralPath (Join-Path $caseDirectory 'config.txt') -Encoding ascii

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = Join-Path $caseDirectory 'csopesy.exe'
    $startInfo.WorkingDirectory = $caseDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    [void]$process.Start()
    try {
        $process.StandardInput.WriteLine('initialize')
        Start-Sleep -Milliseconds 50
        $process.StandardInput.WriteLine('scheduler-start')
        Start-Sleep -Milliseconds $WaitMilliseconds
        $process.StandardInput.WriteLine('report-util')

        $reportPath = Join-Path $caseDirectory 'csopesy-log.txt'
        $deadline = [DateTime]::UtcNow.AddSeconds(3)
        while (-not (Test-Path -LiteralPath $reportPath) -and
               [DateTime]::UtcNow -lt $deadline) {
            Start-Sleep -Milliseconds 20
        }
        Assert-True (Test-Path -LiteralPath $reportPath) `
            "$Name did not create csopesy-log.txt"

        $report = Get-Content -LiteralPath $reportPath -Raw
        $coresUsedMatch = [regex]::Match($report, 'Cores used: (\d+)')
        $coresAvailableMatch = [regex]::Match($report, 'Cores available: (\d+)')
        $utilizationMatch = [regex]::Match($report, 'CPU utilization: (\d+)%')

        Assert-True $coresUsedMatch.Success "$Name omitted cores used"
        Assert-True $coresAvailableMatch.Success "$Name omitted cores available"
        Assert-True $utilizationMatch.Success "$Name omitted CPU utilization"
        Assert-True ([int]$coresUsedMatch.Groups[1].Value -eq $NumCpu) `
            "$Name used $($coresUsedMatch.Groups[1].Value) of $NumCpu configured cores"
        Assert-True ([int]$coresAvailableMatch.Groups[1].Value -eq 0) `
            "$Name incorrectly reported available cores"
        Assert-True ([int]$utilizationMatch.Groups[1].Value -eq 100) `
            "$Name did not report 100% simulated utilization"
        Assert-True ($report -notmatch 'Core:\s*-1') `
            "$Name listed an unassigned/sleeping process as running"

        Write-Host "[PASS] $Name"
    }
    finally {
        if (-not $process.HasExited) {
            $process.StandardInput.WriteLine('exit')
            $process.StandardInput.Close()
            if (-not $process.WaitForExit(5000)) {
                $process.Kill($true)
                [void]$process.WaitForExit(5000)
            }
        }
        $process.Dispose()
        Remove-Item -LiteralPath $caseDirectory -Recurse -Force -ErrorAction SilentlyContinue
    }
}

$cases = @(
    @{ Name = 'rr-1cpu'; NumCpu = 1; Scheduler = 'rr'; Quantum = 5; BatchFrequency = 1; Delay = 0 },
    @{ Name = 'rr-2cpu'; NumCpu = 2; Scheduler = 'rr'; Quantum = 3; BatchFrequency = 1; Delay = 0 },
    @{ Name = 'rr-4cpu'; NumCpu = 4; Scheduler = 'rr'; Quantum = 5; BatchFrequency = 1; Delay = 0 },
    @{ Name = 'rr-8cpu-delay'; NumCpu = 8; Scheduler = 'rr'; Quantum = 7; BatchFrequency = 2; Delay = 3 },
    @{ Name = 'fcfs-4cpu'; NumCpu = 4; Scheduler = 'fcfs'; Quantum = 5; BatchFrequency = 1; Delay = 0 },
    @{ Name = 'fcfs-8cpu-delay'; NumCpu = 8; Scheduler = 'fcfs'; Quantum = 2; BatchFrequency = 2; Delay = 2 }
)

foreach ($case in $cases) {
    Invoke-SchedulerCase @case
}
