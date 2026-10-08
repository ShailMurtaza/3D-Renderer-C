#ifndef TRANSFORMATIONS_H
#define TRANSFORMATIONS_H

#define WIDTH 1920
#define HEIGHT 1080

typedef struct {
  float x, y, z, w;
} Vec4;

typedef struct {
  int a, b;
} Edge;

void rotate_x(Vec4 *vertices, int count, float angle_deg);
void rotate_y(Vec4 *vertices, int count, float angle_deg);
void rotate_z(Vec4 *vertices, int count, float angle_deg);
void translate(Vec4 *vertices, int count, float tx, float ty, float tz);
void project(Vec4 *vertices, int count);
void perspective_divide(Vec4 *v);
void map_to_screen(const Vec4 *v, float *sx, float *sy);
void center_object(Vec4 *vertices, int count);

#endif
