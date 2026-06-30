#!/usr/bin/env bash

set -u

if [[ "$#" -lt 1 ]]; then
  echo "Usage: $0 <executable_path> [more_executables...] [--hosts <hosts_file>]"
  exit 1
fi

HOSTS_FILE="hosts"
REMOTE_DIR="~/local/rgm48935"
EXECUTABLES=()

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --hosts)
      if [[ "$#" -lt 2 ]]; then
        echo "Error: --hosts requires a file path."
        exit 1
      fi
      HOSTS_FILE="$2"
      shift 2
      ;;
    *)
      EXECUTABLES+=("$1")
      shift
      ;;
  esac
done

if [[ "${#EXECUTABLES[@]}" -eq 0 ]]; then
  echo "Error: no executable paths provided."
  exit 1
fi

if [[ ! -f "$HOSTS_FILE" ]]; then
  echo "Error: hosts file '$HOSTS_FILE' not found."
  exit 1
fi

for executable in "${EXECUTABLES[@]}"; do
  if [[ ! -f "$executable" ]]; then
    echo "Error: executable '$executable' not found."
    exit 1
  fi
done

while IFS= read -r line || [[ -n "$line" ]]; do
  line="${line%%#*}"
  line="$(echo "$line" | xargs)"
  [[ -z "$line" ]] && continue

  host="${line%% *}"
  echo "==> $host"

  if ! ssh "$host" "mkdir -p $REMOTE_DIR"; then
    echo "Failed to create remote directory on $host"
    continue
  fi

  for executable in "${EXECUTABLES[@]}"; do
    if rsync -avz "$executable" "${host}:${REMOTE_DIR}/"; then
      echo "  sent $(basename "$executable")"
    else
      echo "  failed $(basename "$executable")"
    fi
  done
done < "$HOSTS_FILE"
