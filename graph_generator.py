import struct
import random
import sys

def Graph_generate_sparse(num_nodes, edge_count, output_path):
    """
    Gera um grafo esparso em formato binário usando streaming para manter memória constante.
    Formato: [int: origem][int: destino][double: peso] (16 bytes por registro)
    """
    binary_format = 'iid'
    buffer_size_bytes = 1024 * 1024

    with open(output_path, 'wb', buffering=buffer_size_bytes) as output_file:
        for edge_id in range(edge_count):
            source_node_id = random.randint(0, num_nodes - 1)
            dest_node_id = random.randint(0, num_nodes - 1)

            while source_node_id == dest_node_id:
                dest_node_id = random.randint(0, num_nodes - 1)

            weight = random.uniform(0.1, 100.0)
            output_file.write(struct.pack(binary_format, source_node_id, dest_node_id, weight))

            if edge_id % 1000000 == 0:
                print(f"Geradas {(edge_id // 1000000)} milhões de arestas...")

    print(f"Geradas com sucesso {edge_count} arestas em '{output_path}'.")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: python graph_generator.py <num_nodes>")
        sys.exit(1)

    try:
        node_count = int(sys.argv[1])
        edge_count = int(node_count * 80)
        Graph_generate_sparse(node_count, edge_count, "graph.bin")
    except ValueError:
        print("Erro: Graph: forneça um número inteiro válido para a quantidade de nós.")
        sys.exit(1)
