#ifndef TRANSFORMATIONS_H
#define TRANSFORMATIONS_H

#define WIDTH 1920
#define HEIGHT 1080

/* Vertical half field of view used by project(), in degrees. */
#define FOV_HALF_DEG 15.0f

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
void project(Vec4 *vertices, int count, float near_plane, float far_plane);
void perspective_divide(Vec4 *v);
void map_to_screen(const Vec4 *v, float *sx, float *sy);
void center_object(Vec4 *vertices, int count);

/* Bounding-sphere radius of the mesh around the origin (call after centering). */
float object_radius(const Vec4 *vertices, int count);

/* Default camera placement for a mesh of the given radius: distance from the
 * object center (positive, camera sits at z = -*cam_dist) plus matching near
 * and far clip planes that enclose the whole object. */
void fit_camera(float radius, float *cam_dist, float *near_plane,
                float *far_plane);

#endif
