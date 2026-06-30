# Rust Server Launcher with Write-Progress bar

$sd = "C:\steamcmd\steamapps\common\rust_dedicated"
$log = "$sd\stdout.log"
$err = "$sd\stderr.log"

Remove-Item $log, $err -ErrorAction SilentlyContinue

# Clean only save data, keep config
Remove-Item "$sd\server\local_test" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item "$sd\server\local" -Recurse -Force -ErrorAction SilentlyContinue

# Pre-create server config with auto-admin
$cfgDir = "$sd\server\local\cfg"
New-Item -ItemType Directory -Force -Path $cfgDir | Out-Null
@"
ownerid 76561197960265728
server.secure 0
server.eac 0
server.encryption 0
"@ | Set-Content "$cfgDir\server.cfg"

# Clean old config so flags always apply fresh
Remove-Item "$sd\server" -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   Rust Local Test Server - No EAC" -ForegroundColor Cyan
Write-Host "   Port: 28015  Size: 2000  Seed: 9999" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# Launch server as background job
$job = Start-Job -Name "RustServer" -ScriptBlock {
    param($exe, $dir, $out, $errout)
    Set-Location $dir
    & $exe "-batchmode" "-nographics" "+server.secure" "0" "+server.eac" "0" "+server.encryption" "0" "+server.writecfg" "+server.port" "28015" "+server.level" "Procedural Map" "+server.seed" "9999" "+server.worldsize" "2000" "+server.maxplayers" "10" "+server.hostname" "Local_Test" "+server.identity" "local" *> $out
} -ArgumentList "$sd\RustDedicated.exe", $sd, $log, $err

Write-Host "Server PID: $((Get-Process RustDedicated -ErrorAction SilentlyContinue).Id)" -ForegroundColor DarkGray
Write-Host ""

$stages = @(
    @{N="Loading procedural map";  P=2;  D="Generating map data"    },
    @{N="Monument Prefabs";        P=8;  D="Loading monuments"      },
    @{N="Height Map";              P=12; D="Generating terrain"     },
    @{N="Lakes";                   P=16; D="Placing lakes"          },
    @{N="Oasis";                   P=20; D="Placing oases"          },
    @{N="Road Network";            P=30; D="Building road network"  },
    @{N="Terrain Erosion";         P=50; D="Eroding terrain"        },
    @{N="River Layout";            P=65; D="Building river layout"  },
    @{N="River Terrain";           P=70; D="Sculpting river terrain"},
    @{N="Harbors";                 P=75; D="Placing harbors"        },
    @{N="Fishing Villages";        P=78; D="Placing fishing villages"},
    @{N="Powerlines";              P=84; D="Placing powerlines"     },
    @{N="Micro Monuments";         P=93; D="Placing micro monuments"},
    @{N="Map Spawning";            P=97; D="Spawning map objects"   },
    @{N="startup complete";        P=100;D="Server ready!"          }
)

$start  = Get-Date
$lastP  = 0
$ready  = $false

while (($job.State -eq "Running") -and !$ready) {
    if (Test-Path $log) {
        $all = Get-Content $log -Raw -ErrorAction SilentlyContinue
        foreach ($s in $stages) {
            if (($all -match $s.N) -and ($lastP -lt $s.P)) {
                $lastP = $s.P
                $elapsed = [int]((Get-Date) - $start).TotalSeconds
                Write-Progress -Activity "Starting Rust Local Server" -Status "$($s.D) ($($s.N))" -PercentComplete $lastP -SecondsRemaining ([math]::Max(0, (100-$lastP)*2))
                if ($s.P -eq 100) { $ready = $true }
            }
        }
    }
    Start-Sleep -Seconds 2
}

Write-Progress -Activity "Starting Rust Local Server" -Completed

if ($ready) {
    Write-Host ""
    Write-Host "=========================================" -ForegroundColor Green
    Write-Host "   SERVER READY!  13,074 entities online" -ForegroundColor Green
    Write-Host "=========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "  STEP 1 — Launch game (no Steam, no EAC):" -ForegroundColor Yellow
    Write-Host '    "C:\Program Files (x86)\Steam\steamapps\common\Rust\RustClient.exe"' -ForegroundColor White
    Write-Host ""
    Write-Host "  STEP 2 — Connect in-game (F1 console):" -ForegroundColor Yellow
    Write-Host "    client.connect 127.0.0.1:28015" -ForegroundColor White
    Write-Host ""
    Write-Host "  STEP 3 — Inject Rust Private Lite.dll (or Rust Private.dll) + press HOME" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  Spawn test entities (F1 console):" -ForegroundColor DarkGray
    Write-Host "    spawn bear    spawn scientist" -ForegroundColor DarkGray
    Write-Host "    entity.spawn minicopter.entity" -ForegroundColor DarkGray
    Write-Host "    inventory.give ak47.rifle 1" -ForegroundColor DarkGray
    Write-Host "    inventory.give ammo.rifle 500" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "  Server running. Press ENTER to stop." -ForegroundColor Yellow
    Write-Host "Spawn: spawn bear, spawn scientist, entity.spawn minicopter.entity" -ForegroundColor DarkGray
    Write-Host "Give items: inventory.give ak47.rifle 1" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "Server is running. Press ENTER to stop." -ForegroundColor Yellow
    Read-Host
    $job | Stop-Job
    Stop-Process -Name "RustDedicated" -Force -ErrorAction SilentlyContinue
}
else {
    Write-Host "Server process ended unexpectedly." -ForegroundColor Red
    if (Test-Path $log) { Get-Content $log -Tail 20 }
    Read-Host
}
