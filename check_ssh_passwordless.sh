#!/usr/bin/env bash

set -u

# Configuration
HOSTS_FILE="${1:-hosts}"
SRC_FILE="/home/local/rgm48935/MPI-Minimum-Spanning-Tree/bin/parallel"
DEST_DIR="/home/local/rgm48935/MPI-Minimum-Spanning-Tree/bin"
SSH_TIMEOUT="${SSH_TIMEOUT:-5}"

# SSH & SCP Options to bypass interactive prompts and host-key checks completely
SSH_OPTS="-n -o BatchMode=yes -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=$SSH_TIMEOUT"
SCP_OPTS="-o BatchMode=yes -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=$SSH_TIMEOUT"

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

echo "Starting deployment of 'parallel' to the cluster (Blind-mode enabled)..."
echo "----------------------------------------------------------------------"

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

  # Step 1: Create directory tree on the remote node securely
  if ssh $SSH_OPTS "$host" "mkdir -p '$DEST_DIR'" >/dev/null 2>&1; then
    
    # Step 2: Copy the file using scp
    if scp $SCP_OPTS "$SRC_FILE" "$host:$DEST_DIR/" < /dev/null >/dev/null 2>&1; then
      echo "  -> [OK] Executable deployed successfully."
      success=$((success + 1))
    else
      echo "  -> [FAIL] mkdir succeeded, but scp transfer failed."
      failed=$((failed + 1))
    fi

  else
    echo "  -> [FAIL] Could not connect or create directory structure."
    failed=$((failed + 1))
  fi
done < "$HOSTS_FILE"

echo "----------------------------------------------------------------------"
echo "Deployment Summary: Total=$total | Success=$success | Failed=$failed"

if [[ "$failed" -gt 0 ]]; then
  exit 1
fi
