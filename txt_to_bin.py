import sys
import struct
import os

def Stream_convert_txt_to_bin(txt_path, bin_path):
    try:
        with open(txt_path, 'r') as txt_file, open(bin_path, 'wb') as bin_file:
            node_count = txt_file.readline().strip()
            edge_count = txt_file.readline().strip()

            print("Processando grafo...")
            print(f"Cabeçalho ignorado -> Nós: {node_count}, Arestas: {edge_count}")

            # Formato binário: origem (uint), destino (uint), peso (double)
            edge_format = '=IId'

            processed_edge_count = 0

            for line_num, line in enumerate(txt_file, start=3):
                line = line.strip()
                if not line:
                    continue

                parts = line.split()
                if len(parts) != 3:
                    print(f"Aviso: linha malformada ignorada na linha {line_num}: '{line}'")
                    continue

                try:
                    source_node_id = int(parts[0])
                    dest_node_id = int(parts[1])
                    weight = float(parts[2])

                    packed_edge = struct.pack(edge_format, source_node_id, dest_node_id, weight)
                    bin_file.write(packed_edge)
                    processed_edge_count += 1

                except ValueError:
                    print(f"Erro: Stream: formato numérico inválido na linha {line_num}. Linha ignorada.")
                    continue

            print(f"Gravadas com sucesso {processed_edge_count} arestas em '{bin_path}'.")

    except FileNotFoundError as error:
        print(f"Erro: Stream: arquivo de entrada '{txt_path}' não encontrado: {error.errno}: {os.strerror(error.errno)}")
    except OSError as error:
        print(f"Erro: Stream: falha de sistema: {error.errno}: {os.strerror(error.errno)}")
    except Exception as error:
        print(f"Erro: Stream: erro inesperado: {error}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Uso: python txt_to_bin.py <grafo_entrada.txt> <grafo_saida.bin>")
        sys.exit(1)

    Stream_convert_txt_to_bin(sys.argv[1], sys.argv[2])
