param([int]$ExpectedRunResult = 0)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$exePath = Join-Path $repoRoot 'ProjectName/x64/Debug/ExitDiagnostics.exe'
$logDir = Join-Path $repoRoot '.verification/exit-results'
New-Item -ItemType Directory -Force $logDir | Out-Null

# The diagnostic program owns its quit request; this runner sends no UI input.
foreach ($mode in @('quit', 'play', 'leak')) {
    $stdout = Join-Path $logDir "$mode.txt"
    $stderr = Join-Path $logDir "$mode-crt.txt"
    $process = Start-Process -FilePath $exePath -ArgumentList "--$mode" `
        -WorkingDirectory (Split-Path $exePath -Parent) -WindowStyle Hidden `
        -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    if (!$process.WaitForExit(15000)) {
        throw "Diagnostic timeout: mode=$mode PID=$($process.Id)"
    }
    $process.Refresh()
    $output = Get-Content $stdout -Raw
    $report = Get-Content $stderr -Raw
    $marker = 'Automatic CRT report after static destruction follows:'
    if (!$report.Contains($marker)) { throw "Missing final marker: $mode" }
    $finalReport = $report.Substring($report.IndexOf($marker) + $marker.Length).Trim()
    if ($process.ExitCode -ne $ExpectedRunResult -or
        $output -notmatch "(?m)^Run=$ExpectedRunResult ") {
        throw "Unexpected run result: $mode exit=$($process.ExitCode) $output"
    }
    if ($mode -eq 'leak') {
        if ($finalReport -notmatch '37 bytes long' -or
            $finalReport -notmatch 'Detected memory leaks!') {
            throw 'Positive control was not detected after static destruction'
        }
    } elseif ($finalReport.Length -ne 0) {
        throw "Unexpected final CRT report: $mode $finalReport"
    }
    Write-Output "$mode PASS: exit=$($process.ExitCode); final CRT leak report=$($mode -eq 'leak')"
    Write-Output $output.Trim()
}
