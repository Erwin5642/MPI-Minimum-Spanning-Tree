USER=$(whoami)
HOSTS_FILE="hosts"
WORKSPACE_DIR="$HOME/../../local/rgm48935/MPI-Minimum-Spanning-Tree"
 
if [ ! -f "$HOSTS_FILE" ]; then
     echo "Error: $HOSTS_FILE file not found!"
     exit 1
 fi
 
echo "Starting Sync and Build across the cluster..."
 
while IFS= read <&3 -r HOST || [ -n "$HOST" ]; do
    [[ -z "$HOST" || "$HOST" == \#* ]] && continue

    echo " Processing $HOST..."

    if [ -d "src" ] && [ -f Makefile ]; then

        echo "Syncing files..."
        rsync -avz src Makefile hosts mpi_hosts "$USER@$HOST:$WORKSPACE_DIR/"

        echo "Compiling program..."
        ssh "$USER@$HOST" "cd $WORKSPACE_DIR && make"
 
    else
        echo "Warning: 'src' directory or 'Makefile' missing locally. Skipping $HOST."
    fi

done 3< "$HOSTS_FILE"

echo "Cluster is fully synced and compiled!"
