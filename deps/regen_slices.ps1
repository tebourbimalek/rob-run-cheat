# regen_slices.ps1 — regenerate deps/slices/* from deps/dump.cs (BUILD_ID 26807668)
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File deps/regen_slices.ps1
# Rule: never load whole dump into editor; this script streams line ranges.

$ErrorActionPreference = 'Stop'
$dump = Join-Path $PSScriptRoot 'dump.cs'
$outDir = Join-Path $PSScriptRoot 'slices'
if (!(Test-Path $dump)) { throw "missing $dump" }
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

function Write-Slice($name, $pattern, $before=5, $after=180) {
  $m = Select-String -Path $dump -Pattern $pattern -SimpleMatch | Select-Object -First 1
  if (!$m) { Write-Warning "MISS: $pattern"; return }
  $start = [Math]::Max(1, $m.LineNumber - $before)
  $end = $m.LineNumber + $after
  $lines = Get-Content $dump -TotalCount $end | Select-Object -Skip ($start - 1)
  $path = Join-Path $outDir $name
  $lines | Set-Content -Path $path -Encoding UTF8
  Write-Host "OK $name <- line $($m.LineNumber)"
}

Write-Slice 'BattleEntitiesModel.txt' 'public class BattleEntitiesModel : IBattleEntitiesModel'
Write-Slice 'BattleModel.txt'         'public class BattleModel // TypeDefIndex: 5527'
Write-Slice 'PlayerEntityModel.txt'   'public class PlayerEntityModel : EntityModel'
Write-Slice 'PlayerClientModel.txt'   'public class PlayerClientModel : CharacterClientModel'
Write-Slice 'CharacterClientModel.txt' 'public abstract class CharacterClientModel // TypeDefIndex: 9059'
Write-Slice 'Camera_UnityEngine.txt'  'public sealed class Camera : Behaviour'
Write-Slice 'Transform.txt'           'public class Transform : Component, IEnumerable'
Write-Slice 'SkinnedMeshRenderer.txt' 'public class SkinnedMeshRenderer : Renderer'
Write-Slice 'Animator_GetBoneTransform.txt' 'public class Animator : Behaviour'
Write-Slice 'Physics_Raycast.txt'     'public class Physics // TypeDefIndex: 36047'
Write-Slice 'EntityModel.txt'         'public abstract class EntityModel : IEntityModel // TypeDefIndex: 8472' 5 120
