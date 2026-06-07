import os
import numpy as np
from scipy.sparse import csr_matrix
from scipy.sparse.csgraph import minimum_spanning_tree

def MST_calculate_from_file(file_path: str) -> float:
    """
    Calcula o custo total da árvore geradora mínima a partir de um arquivo de texto.
    """
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"Erro: MST: arquivo '{file_path}' não existe.")

    with open(file_path, 'r') as input_file:
        node_count = int(input_file.readline().strip())
        edge_count = int(input_file.readline().strip())

    # skiprows=2 ignora o cabeçalho com contagens de nós e arestas
    edge_data = np.loadtxt(file_path, skiprows=2)

    source_node_ids = edge_data[:, 0].astype(np.int32)
    dest_node_ids = edge_data[:, 1].astype(np.int32)
    edge_weights = edge_data[:, 2]

    graph_matrix = csr_matrix((edge_weights, (source_node_ids, dest_node_ids)), shape=(node_count, node_count))

    mst_matrix = minimum_spanning_tree(graph_matrix)

    return float(mst_matrix.sum())

if __name__ == "__main__":
    sample_file_path = "data/dados_entrada_sequencial.txt"

    total_cost = MST_calculate_from_file(sample_file_path)
    print(f"Custo total da Árvore Geradora Mínima: {total_cost}")
