#!/usr/bin/env bash

set -u

# Configuration
HOSTS_FILE="${1:-hosts}"
SRC_FILE="/home/local/rgm48935/MPI-Minimum-Spanning-Tree/bin/parallel"
DEST_DIR="/home/local/rgm48935/MPI-Minimum-Spanning-Tree/bin"
SSH_TIMEOUT="${SSH_TIMEOUT:-5}"

# Check if the local executable actually exists before starting
if [[ ! -f "$SRC_FILE" ]]; then
  echo "Erro: Local executable '$SRC_FILE' not found."
  exit 1
fi

# Check if host file exists
if [[ ! -f "$HOSTS_FILE" ]]; then
  echo "Erro: Hosts file '$HOSTS_FILE' not found."
  exit 1
fi

echo "Starting deployment of 'parallel' to the cluster..."
echo "--------------------------------------------------"

total=0
success=0
failed=0

while IFS= read -r line || [[ -n "$line" ]]; do
  # Strip comments and whitespace
  line="${line%%#*}"
  line="$(echo "$line" | xargs)"
  [[ -z "$line" ]] && continue

  host="$line"
  total=$((total + 1))
  
  echo "[Node] $host"

  # Added "-n" to ssh to prevent it from eating the stdin/hosts file
  if ssh -n -o BatchMode=yes -o ConnectTimeout="$SSH_TIMEOUT" "$host" "mkdir -p '$DEST_DIR'" >/dev/null 2>&1; then
    
    # Added "< /dev/null" to scp just to be completely safe against the same issue
    if scp -o BatchMode=yes -o ConnectTimeout="$SSH_TIMEOUT" "$SRC_FILE" "$host:$DEST_DIR/" < /dev/null >/dev/null 2>&1; then
      echo "  -> [OK] Executable deployed successfully."
      success=$((success + 1))
    else
      echo "  -> [FAIL] mkdir succeeded, but scp transfer failed."
      failed=$((failed + 1))
    fi

  else
    echo "  -> [FAIL] Could not create directory structure (Check passwordless SSH connection)."
    failed=$((failed + 1))
  fi
done < "$HOSTS_FILE"

echo "--------------------------------------------------"
echo "Deployment Summary: Total=$total | Success=$success | Failed=$failed"

if [[ "$failed" -gt 0 ]]; then
  exit 1
fi
