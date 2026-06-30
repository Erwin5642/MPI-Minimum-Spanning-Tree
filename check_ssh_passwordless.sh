#!/usr/bin/env bash

set -u

HOSTS_FILE="${1:-hosts}"
SSH_TIMEOUT="${SSH_TIMEOUT:-5}"
# Define o caminho padrão para a chave (preferência por Ed25519, mais moderna e segura)
KEY_FILE="$HOME/.ssh/id_ed25519"

if [[ ! -f "$HOSTS_FILE" ]]; then
  echo "Erro: Arquivo de hosts '$HOSTS_FILE' não encontrado."
  exit 1
fi

# 1. Garante que o nó mestre possui um par de chaves SSH
if [[ ! -f "$KEY_FILE" && ! -f "$HOME/.ssh/id_rsa" ]]; then
  echo "Nenhuma chave SSH encontrada na máquina local."
  echo "Gerando uma nova chave Ed25519 (sem senha) para o cluster..."
  mkdir -p "$HOME/.ssh"
  chmod 700 "$HOME/.ssh"
  ssh-keygen -t ed25519 -N "" -f "$KEY_FILE"
fi

total=0
ok=0
fail=0
fixed=0

while IFS= read -r line || [[ -n "$line" ]]; do
  line="${line%%#*}"
  line="$(echo "$line" | xargs)"
  [[ -z "$line" ]] && continue

  host="${line%% *}"
  total=$((total + 1))

  # Testa a conexão sem senha
  if ssh -o BatchMode=yes -o ConnectTimeout="$SSH_TIMEOUT" "$host" "true" >/dev/null 2>&1; then
    echo "[OK] $host (Acesso sem senha já configurado)"
    ok=$((ok + 1))
  else
    echo "[Tentando Corrigir] $host não aceita SSH sem senha. Iniciando pareamento..."
    
    # O ssh-copy-id vai pedir a senha do usuário remoto para copiar a chave pública
    if ssh-copy-id -o ConnectTimeout="$SSH_TIMEOUT" "$host"; then
      
      # Valida se a cópia realmente resolveu o problema
      if ssh -o BatchMode=yes -o ConnectTimeout="$SSH_TIMEOUT" "$host" "true" >/dev/null 2>&1; then
        echo "[CORRIGIDO] $host agora está acessível sem senha!"
        ok=$((ok + 1))
        fixed=$((fixed + 1))
      else
        echo "[ERRO] Chave copiada para $host, mas o teste de conexão ainda falhou."
        fail=$((fail + 1))
      fi
    else
      echo "[FALHA] Não foi possível enviar a chave para $host (Senha incorreta ou timeout)."
      fail=$((fail + 1))
    fi
  fi
done < "$HOSTS_FILE"

echo
echo "=== Resumo ==="
echo "Total de nós analisados: $total"
echo "Acessíveis / Corrigidos: $ok (Sendo $fixed corrigidos agora)"
echo "Falhas restantes:        $fail"

if [[ "$fail" -gt 0 ]]; then
  echo "Aviso: O cluster possui nós problemáticos. Corrija-os antes de rodar o mpirun."
  exit 1
else
  echo "Sucesso! Todos os nós estão prontos para o OpenMPI."
fi