<#
Build an AnimeStudio asset index without exporting every asset.

The decrypted VFS tree remains the only full-size working copy. AnimeStudio's
map-only mode reads the .ab files and writes a compact asset map; conversion is
deferred until a candidate bundle has been selected.

Example:
  .\tools\index_endfield_bundles.ps1 `
    -InputPath 'D:\EndfieldExtracts\v1\Bundles' `
    -AnimeStudioPath 'C:\tools\EndfieldUnpacker\AnimeStudio-net10\AnimeStudio.CLI.exe' `
    -OutputPath 'D:\EndfieldExtracts\v1\anime-index'
#>

[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)]
  [string] $InputPath,

  [Parameter(Mandatory = $true)]
  [string] $AnimeStudioPath,

  [Parameter(Mandatory = $true)]
  [string] $OutputPath,

  [string] $DotnetRoot = '',

  [ValidateSet('ArknightsEndfield', 'ArknightsEndfieldCB1', 'ArknightsEndfieldCB2', 'ArknightsEndfieldCB3')]
  [string] $Game = 'ArknightsEndfield'
)

$ErrorActionPreference = 'Stop'

function Resolve-ExistingFile([string] $Path, [string] $Description) {
  $resolved = Resolve-Path -LiteralPath $Path -ErrorAction SilentlyContinue
  if ($null -eq $resolved -or -not (Test-Path -LiteralPath $resolved.Path -PathType Leaf)) {
    throw "$Description was not found: $Path"
  }
  return $resolved.Path
}

function Resolve-ExistingDirectory([string] $Path, [string] $Description) {
  $resolved = Resolve-Path -LiteralPath $Path -ErrorAction SilentlyContinue
  if ($null -eq $resolved -or -not (Test-Path -LiteralPath $resolved.Path -PathType Container)) {
    throw "$Description was not found: $Path"
  }
  return $resolved.Path
}

$inputRoot = Resolve-ExistingDirectory $InputPath 'Input directory'
$animeExe = Resolve-ExistingFile $AnimeStudioPath 'AnimeStudio CLI'
$outputRoot = [System.IO.Path]::GetFullPath($OutputPath)
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$logPath = Join-Path $outputRoot 'animestudio-index.log'

$oldDotnetRoot = $env:DOTNET_ROOT
try {
  if ($DotnetRoot) {
    $env:DOTNET_ROOT = (Resolve-ExistingDirectory $DotnetRoot '.NET root')
  }

  Write-Host "Indexing $inputRoot"
  Write-Host "Output: $outputRoot"
  & $animeExe $inputRoot $outputRoot `
    --game $Game `
    --map_op Both `
    --map_type XML `
    --map_name assets_map `
    --export_type Raw `
    --silent *> $logPath
  $exitCode = $LASTEXITCODE
  if ($exitCode -ne 0) {
    throw "AnimeStudio failed with exit code $exitCode. See $logPath"
  }
}
finally {
  if ($null -eq $oldDotnetRoot) {
    Remove-Item Env:DOTNET_ROOT -ErrorAction SilentlyContinue
  }
  else {
    $env:DOTNET_ROOT = $oldDotnetRoot
  }
}

$mapPath = Join-Path $outputRoot 'assets_map.xml'
if (-not (Test-Path -LiteralPath $mapPath -PathType Leaf)) {
  throw "AnimeStudio completed without creating $mapPath"
}

$map = Get-Item -LiteralPath $mapPath
Write-Host ("Index complete: {0} ({1} bytes)" -f $map.FullName, $map.Length)
