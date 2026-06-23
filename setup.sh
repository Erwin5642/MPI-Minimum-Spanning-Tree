#!/bin/bash

USER=$(whoami)
HOSTS_FILE="hosts"
WORKSPACE_DIR="$HOME/local/mpi"
KEY_PATH="$HOME/.ssh/id_ed25519"

[ ! -f "$KEY_PATH" ] && ssh-keygen -t ed25519 -f "$KEY_PATH" -N "" -q

[ ! -f "$HOSTS_FILE" ] && exit 1

while IFS= read -r HOST || [ -n "$HOST" ]; do
   [[ -z "$HOST" || "$HOST" == \#* ]] && continue
   ssh-copy-id -i "$KEY_PATH.pub" "$USER@$HOST"

   ssh "$USER@$HOST" "mkdir -p $WORKSPACE_DIR"
   
   [ -d "src" ] && [ -f Makefile ] && rsync -avz src Makefile "$USER@$HOST:$WORKSPACE_DIR/"

   ssh "$USER@$HOST" "cd $WORKSPACE_DIR && make parallel"

done < "$HOSTS_FILE"



