import argparse
import struct
import sys

EDGE_STRUCT = struct.Struct("<IId")
BUFFER_SIZE_BYTES = 1024 * 1024
DEFAULT_EDGES_PER_NODE = 80


def Graph_write_edge(output_file, source_node_id, dest_node_id, weight):
    output_file.write(EDGE_STRUCT.pack(source_node_id, dest_node_id, weight))


def Graph_generate_connected(num_nodes, edge_count, output_path, seed=None):
    """
    Gera um grafo conexo esparso em formato binário, com topologia semelhante ao
    setup_cod_paralelo: arestas aleatórias entre vértices, pesos uniformes em [0, 1).

    Formato por aresta: origem (uint32), destino (uint32), peso (double) — 16 bytes.
    """
    if num_nodes < 1:
        raise ValueError("num_nodes deve ser >= 1")
    if edge_count < num_nodes - 1:
        raise ValueError("edge_count deve ser >= num_nodes - 1 para permitir grafo conexo")

    rng = __import__("random").Random(seed)

    with open(output_path, "wb", buffering=BUFFER_SIZE_BYTES) as output_file:
        for node_id in range(1, num_nodes):
            parent_node_id = rng.randrange(node_id)
            Graph_write_edge(output_file, parent_node_id, node_id, rng.random())

            if node_id % 1_000_000 == 0:
                print(f"Árvore geradora: {node_id // 1_000_000} milhões de nós conectados...")

        remaining_edge_count = edge_count - (num_nodes - 1)
        for edge_id in range(remaining_edge_count):
            source_node_id = rng.randrange(num_nodes)
            dest_node_id = rng.randrange(num_nodes)
            while dest_node_id == source_node_id:
                dest_node_id = rng.randrange(num_nodes)

            Graph_write_edge(output_file, source_node_id, dest_node_id, rng.random())

            if edge_id % 1_000_000 == 0:
                print(f"Arestas aleatórias: {edge_id // 1_000_000} milhões geradas...")

    print(f"Geradas com sucesso {edge_count} arestas conexas em '{output_path}'.")


def Graph_parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Gera grafo binário conexo no formato do setup_cod_paralelo "
            "(uint32 origem, uint32 destino, double peso)."
        )
    )
    parser.add_argument("num_nodes", type=int, help="Quantidade de vértices")
    parser.add_argument(
        "edge_count",
        type=int,
        nargs="?",
        help=f"Quantidade de arestas (padrão: num_nodes * {DEFAULT_EDGES_PER_NODE})",
    )
    parser.add_argument(
        "output_path",
        nargs="?",
        default="graph.bin",
        help="Arquivo de saída (padrão: graph.bin)",
    )
    parser.add_argument("--seed", type=int, default=None, help="Semente para reprodutibilidade")
    return parser.parse_args()


if __name__ == "__main__":
    args = Graph_parse_args()

    try:
        edge_count = args.edge_count if args.edge_count is not None else args.num_nodes * DEFAULT_EDGES_PER_NODE
        Graph_generate_connected(args.num_nodes, edge_count, args.output_path, seed=args.seed)
    except ValueError as error:
        print(f"Erro: Graph: {error}")
        sys.exit(1)
