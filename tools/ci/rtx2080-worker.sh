#!/usr/bin/env bash
set -euo pipefail

WIN_IP="${RTX2080_WIN_IP:-10.0.0.12}"
WIN_MAC="${RTX2080_WIN_MAC:-EC:75:0C:66:E4:3A}"
WIN_USER="${RTX2080_WIN_USER:-PC}"
WIN_KEY="${RTX2080_WIN_KEY:-$HOME/.ssh/remotecomputer_worker_ed25519}"
WIN_KNOWN_HOSTS="${RTX2080_WIN_KNOWN_HOSTS:-$HOME/.ssh/remotecomputer_worker_known_hosts}"
WIN_RUNNER_SERVICE="${RTX2080_WIN_RUNNER_SERVICE:-actions.runner.FlyzDev-avidemux2.RTX2080-avidemux2}"
VM_NAME="${RTX2080_LINUX_VM_NAME:-REA-Ubuntu}"
GUEST_IP="${RTX2080_LINUX_GUEST_IP:-192.168.250.2}"
GUEST_PORT="${RTX2080_LINUX_GUEST_PORT:-2222}"
GUEST_USER="${RTX2080_LINUX_GUEST_USER:-flyzci}"
GUEST_KEY="${RTX2080_LINUX_GUEST_KEY:-$HOME/.ssh/rea_linux_vm_ed25519}"
GUEST_KNOWN_HOSTS="${RTX2080_LINUX_GUEST_KNOWN_HOSTS:-$HOME/.ssh/rea_linux_vm_known_hosts}"
LINUX_RUNNER_SERVICE="${RTX2080_LINUX_RUNNER_SERVICE:-actions.runner.FlyzDev-avidemux2.RTX2080-Ubuntu-avidemux2.service}"
READY_TIMEOUT_SECONDS="${RTX2080_READY_TIMEOUT_SECONDS:-180}"

win_ssh() {
  ssh -T -i "$WIN_KEY" \
    -o BatchMode=yes -o PasswordAuthentication=no -o KbdInteractiveAuthentication=no \
    -o StrictHostKeyChecking=yes -o "UserKnownHostsFile=$WIN_KNOWN_HOSTS" \
    -o ConnectTimeout=5 "$WIN_USER@$WIN_IP" "$@"
}

guest_ssh() {
  ssh -T -p "$GUEST_PORT" -i "$GUEST_KEY" \
    -o BatchMode=yes -o PasswordAuthentication=no -o KbdInteractiveAuthentication=no \
    -o StrictHostKeyChecking=yes -o "UserKnownHostsFile=$GUEST_KNOWN_HOSTS" \
    -o ConnectTimeout=5 "$GUEST_USER@$WIN_IP" "$@"
}

win_ps() {
  local encoded
  encoded=$(PS_SCRIPT="\$ProgressPreference='SilentlyContinue'; $1" python3 -c 'import os,base64; print(base64.b64encode(os.environ["PS_SCRIPT"].encode("utf-16le")).decode())')
  win_ssh powershell.exe -NoProfile -NonInteractive -EncodedCommand "$encoded"
}

probe_win() {
  win_ssh cmd.exe /d /c exit 0 >/dev/null 2>&1
}

probe_guest() {
  guest_ssh uname -m 2>/dev/null | grep -qx x86_64
}

write_output() {
  if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
    printf '%s=%s\n' "$1" "$2" >> "$GITHUB_OUTPUT"
  fi
}

send_wol() {
  RTX2080_MAC="$WIN_MAC" python3 - <<'PY'
import os, socket, time
mac = bytes.fromhex(os.environ["RTX2080_MAC"].replace(":", "").replace("-", ""))
packet = b"\xff" * 6 + mac * 16
for host in ("255.255.255.255", "10.0.0.255"):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    for _ in range(3):
        s.sendto(packet, (host, 9))
        time.sleep(0.2)
    s.close()
PY
}

wait_for_win() {
  local deadline=$((SECONDS + READY_TIMEOUT_SECONDS))
  until probe_win; do
    (( SECONDS >= deadline )) && { echo "Windows SSH readiness timeout" >&2; return 1; }
    sleep 2
  done
}

ensure_win_runner() {
  win_ps "\$s=Get-Service -Name '$WIN_RUNNER_SERVICE' -ErrorAction Stop; if(\$s.Status -ne 'Running'){Start-Service -Name \$s.Name}; \$s=Get-Service -Name \$s.Name; if(\$s.Status -ne 'Running'){exit 21}; Write-Output \$s.Status" >/dev/null
}

ensure_portproxy() {
  win_ps "netsh interface portproxy delete v4tov4 listenaddress=$WIN_IP listenport=$GUEST_PORT | Out-Null; netsh interface portproxy add v4tov4 listenaddress=$WIN_IP listenport=$GUEST_PORT connectaddress=$GUEST_IP connectport=22 | Out-Null; Start-Sleep -Milliseconds 400; if(-not (Get-NetTCPConnection -LocalAddress '$WIN_IP' -LocalPort $GUEST_PORT -State Listen -ErrorAction SilentlyContinue)){exit 31}" >/dev/null
}

wake() {
  local woke=false
  if ! probe_win; then
    send_wol
    wait_for_win
    woke=true
  fi
  ensure_win_runner
  write_output woke "$woke"
  printf '{"woke":%s,"sshReady":true,"runnerServiceReady":true}\n' "$woke"
}

linux_start() {
  probe_win || { echo "Windows host is offline; run wake first" >&2; return 1; }
  ensure_portproxy
  win_ps "\$vm=Get-VM -Name '$VM_NAME' -ErrorAction Stop; if(\$vm.State -eq 'Off'){Start-VM -Name \$vm.Name | Out-Null}; Write-Output \$vm.State" >/dev/null
  local deadline=$((SECONDS + READY_TIMEOUT_SECONDS))
  until probe_guest; do
    (( SECONDS >= deadline )) && { echo "Ubuntu guest readiness timeout" >&2; return 1; }
    sleep 2
  done
  guest_ssh systemctl is-active "$LINUX_RUNNER_SERVICE" | grep -qx active
  docker_status=$(guest_ssh docker version --format 'server={{.Server.Version}}' 2>/dev/null || true)
  [[ "$docker_status" == server=* ]] || { echo "Docker is not ready for CI" >&2; return 1; }
  printf '{"guestReady":true,"runnerServiceReady":true,"dockerReady":true}\n'
}

human_use() {
  local raw
  raw=$(win_ssh powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File 'C:\ProgramData\RemoteComputer\worker-agent.ps1' status-json 2>/dev/null) || return 1
  STATUS_JSON="$raw" python3 -c 'import os,json; print(json.loads(os.environ["STATUS_JSON"].splitlines()[-1]).get("humanUse","unknown"))'
}

windows_worker_count() {
  win_ps "Write-Output @(Get-Process -Name 'Runner.Worker' -ErrorAction SilentlyContinue).Count" 2>/dev/null | tr -d '\r' | tail -1
}

linux_worker_busy() {
  probe_guest || return 1
  guest_ssh sh -lc "pgrep -f '/bin/Runner.Worker' >/dev/null" >/dev/null 2>&1
}

poweroff_if_idle() {
  if ! probe_win; then
    echo '{"poweredOff":false,"alreadyOffline":true}'
    return 0
  fi
  local use count
  use=$(human_use) || { echo '{"poweredOff":false,"reason":"human-use-unknown"}'; return 0; }
  if [[ "$use" != "inactive" ]]; then
    printf '{"poweredOff":false,"reason":"human-use-%s"}\n' "$use"
    return 0
  fi
  for _ in 1 2; do
    count=$(windows_worker_count || echo unknown)
    if [[ "$count" != "0" ]]; then
      printf '{"poweredOff":false,"reason":"windows-runner-busy","count":"%s"}\n' "$count"
      return 0
    fi
    if linux_worker_busy; then
      echo '{"poweredOff":false,"reason":"linux-runner-busy"}'
      return 0
    fi
    sleep 3
  done
  win_ssh shutdown.exe /s /t 0 /f >/dev/null 2>&1 || true
  for _ in $(seq 1 30); do
    if ! probe_win; then
      echo '{"poweredOff":true,"confirmedByDisconnect":true}'
      return 0
    fi
    sleep 1
  done
  echo "Windows host stayed reachable after shutdown request" >&2
  return 1
}

case "${1:-}" in
  wake) wake ;;
  linux-start) linux_start ;;
  poweroff-if-idle) poweroff_if_idle ;;
  *) echo "usage: $0 wake|linux-start|poweroff-if-idle" >&2; exit 2 ;;
esac
