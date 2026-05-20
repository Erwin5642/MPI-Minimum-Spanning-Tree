## Implementação do Problema Sequencial


### Estrutura de dados pro grafo

- Graph Representation 
type Weight :: double;
type NodeId :: unsigned int;

type Edge :: { 
    source :: NodeId;
    destination :: NodeId;
    weight :: Weight;
}

- Components
type DisjointSetPool :: { 
    parents :: Array<NodeId>; 
    ranks :: Array<unsigned int>;
    nodeCount :: UnsignedInt;
} 

- Current best edge by component
type CheapestEdges :: Array<Edge>

### Tasks

1. Stream File
2. Allocate memory for EdgeList
3. Allocate memory for UnionFind
4. Allocate memory for CheapestsEdges
5. Write Edges from file to EdgeList
6. Initialize each entry in UnionFind with self
7. Initialize each entry in CheapestsEdges with Nothing
8. Yield edges on graph
9. Check edge is member of different components
10. Update best edge for each vertex of the edge

### Strategy

