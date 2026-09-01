<#
.SYNOPSIS
  Copy Bundle files listed by an EIEM capture manifest from a full decrypted tree.

.DESCRIPTION
  This script does not read or decrypt the game's VFS. It only copies files from
  an existing output produced by extract_endfield_vfs.ps1/decrypt_vfs.py.
#>

[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)]
  [string]$ManifestPath,

  [Parameter(Mandatory = $true)]
  [string]$DecryptedRoot,

  [Parameter(Mandatory = $true)]
  [string]$OutputPath
)

$manifest = Get-Content -Raw -LiteralPath $ManifestPath | ConvertFrom-Json
$sourceRoot = (Resolve-Path -LiteralPath $DecryptedRoot).Path
$destinationRoot = [System.IO.Path]::GetFullPath($OutputPath)
New-Item -ItemType Directory -Path $destinationRoot -Force | Out-Null

$copied = 0
$missing = 0
[long]$bytes = 0
$failures = [System.Collections.Generic.List[object]]::new()
$seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

foreach ($bundle in $manifest.bundles) {
  $logical = [string]$bundle.path
  if ([string]::IsNullOrWhiteSpace($logical) -or !$seen.Add($logical)) {
    continue
  }

  # The decrypted tree strips the leading Data/ component.
  $relative = $logical -replace '^Data[/\\]', ''
  $relative = $relative -replace '/', '\\'
  $source = Join-Path $sourceRoot $relative
  $destination = Join-Path $destinationRoot $relative

  if (!(Test-Path -LiteralPath $source -PathType Leaf)) {
    $missing++
    $failures.Add([pscustomobject]@{ path = $logical; error = 'source file not found' })
    continue
  }

  try {
    $destinationDirectory = Split-Path -Parent $destination
    New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
    $length = (Get-Item -LiteralPath $source).Length
    $bytes += [long]$length
    $copied++
  } catch {
    $failures.Add([pscustomobject]@{ path = $logical; error = $_.Exception.Message })
  }
}

$report = [pscustomobject]@{
  schema = 1
  kind = 'scene_bundle_copy'
  manifest = (Resolve-Path -LiteralPath $ManifestPath).Path
  source = $sourceRoot
  output = $destinationRoot
  requested = $seen.Count
  copied = $copied
  missingOrFailed = $missing + ($failures.Count - $missing)
  bytes = $bytes
  failures = $failures
}
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $destinationRoot 'copy-report.json') -Encoding utf8
Write-Host ("Copied {0}/{1} files ({2} bytes). Report: {3}" -f $copied, $seen.Count, $bytes, (Join-Path $destinationRoot 'copy-report.json'))
if ($failures.Count -gt 0) { exit 2 }
