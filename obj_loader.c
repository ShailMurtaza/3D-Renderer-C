#include "obj_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void add_vertex(Mesh *mesh, float x, float y, float z)
{
    if (mesh->vertex_count >= mesh->vertex_capacity) {
        mesh->vertex_capacity = mesh->vertex_capacity ? mesh->vertex_capacity * 2 : 64;
        mesh->vertices = realloc(mesh->vertices, mesh->vertex_capacity * sizeof(Vec4));
    }
    int i = mesh->vertex_count++;
    mesh->vertices[i].x = x;
    mesh->vertices[i].y = y;
    mesh->vertices[i].z = z;
    mesh->vertices[i].w = 1.0f;
}

static void add_edge(Mesh *mesh, int a, int b)
{
    if (a == b) return;
    if (a > b) { int t = a; a = b; b = t; }
    if (mesh->edge_count >= mesh->edge_capacity) {
        mesh->edge_capacity = mesh->edge_capacity ? mesh->edge_capacity * 2 : 128;
        mesh->edges = realloc(mesh->edges, mesh->edge_capacity * sizeof(Edge));
    }
    for (int i = 0; i < mesh->edge_count; i++) {
        if (mesh->edges[i].a == a && mesh->edges[i].b == b)
            return;
    }
    int i = mesh->edge_count++;
    mesh->edges[i].a = a;
    mesh->edges[i].b = b;
}

static int parse_vertex_index(const char *token)
{
    char buf[64];
    int i = 0;
    while (*token && *token != '/' && i < 63)
        buf[i++] = *token++;
    buf[i] = '\0';
    return atoi(buf) - 1;
}

Mesh *load_obj(const char *filename)
{
    Mesh *mesh = calloc(1, sizeof(Mesh));
    if (!mesh) return NULL;

    FILE *f = fopen(filename, "r");
    if (!f) {
        free(mesh);
        return NULL;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == 'v' && line[1] == ' ') {
            float x, y, z;
            if (sscanf(line, "v %f %f %f", &x, &y, &z) >= 3)
                add_vertex(mesh, x, y, z);
        } else if (line[0] == 'f' && line[1] == ' ') {
            char *save = NULL;
            char *token = strtok_r(line + 2, " \t\r\n", &save);
            int indices[64];
            int idx_count = 0;
            while (token && idx_count < 64) {
                indices[idx_count++] = parse_vertex_index(token);
                token = strtok_r(NULL, " \t\r\n", &save);
            }
            for (int i = 0; i < idx_count; i++)
                add_edge(mesh, indices[i], indices[(i + 1) % idx_count]);
        }
    }

    fclose(f);
    return mesh;
}

void free_mesh(Mesh *mesh)
{
    if (mesh) {
        free(mesh->vertices);
        free(mesh->edges);
        free(mesh);
    }
}
