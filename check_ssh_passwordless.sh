#!/usr/bin/env bash

set -u

HOSTS_FILE="${1:-hosts}"
SSH_TIMEOUT="${SSH_TIMEOUT:-5}"

if [[ ! -f "$HOSTS_FILE" ]]; then
  echo "Error: hosts file '$HOSTS_FILE' not found."
  exit 1
fi

total=0
ok=0
fail=0

while IFS= read -r line || [[ -n "$line" ]]; do
  line="${line%%#*}"
  line="$(echo "$line" | xargs)"
  [[ -z "$line" ]] && continue

  host="${line%% *}"
  total=$((total + 1))

  if ssh -o BatchMode=yes -o ConnectTimeout="$SSH_TIMEOUT" "$host" "true" >/dev/null 2>&1; then
    echo "[OK] $host"
    ok=$((ok + 1))
  else
    echo "[FAIL] $host (passwordless SSH not working)"
    fail=$((fail + 1))
  fi
done < "$HOSTS_FILE"

echo
echo "Summary: total=$total ok=$ok fail=$fail"

if [[ "$fail" -gt 0 ]]; then
  exit 1
fi
