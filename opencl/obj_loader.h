#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include "transformations.h"

typedef struct {
    Vec4 *vertices;
    int vertex_count;
    int vertex_capacity;
    Edge *edges;
    int edge_count;
    int edge_capacity;
} Mesh;

Mesh *load_obj(const char *filename);
void free_mesh(Mesh *mesh);

#endif
