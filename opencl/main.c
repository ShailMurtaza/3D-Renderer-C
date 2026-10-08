#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clip.h"
#include "events.h"
#include "obj_loader.h"
#include "opencl_utils.h"
#include "transformations.h"

static float cam_x = 0.0f, cam_y = 0.0f, cam_z = -50.0f;

static void draw_edge(SDL_Renderer *renderer, Vec4 p1, Vec4 p2) {
  perspective_divide(&p1);
  perspective_divide(&p2);

  float sx1, sy1, sx2, sy2;
  map_to_screen(&p1, &sx1, &sy1);
  map_to_screen(&p2, &sx2, &sy2);

  SDL_RenderLine(renderer, sx1, sy1, sx2, sy2);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "No obj file provided!\n");
    return 1;
  }
  const char *obj_path = argv[1];

  Mesh *mesh = load_obj(obj_path);
  if (!mesh) {
    fprintf(stderr, "Failed to load OBJ file: %s\n", obj_path);
    return 1;
  }

  center_object(mesh->vertices, mesh->vertex_count);

  Vec4 *working = NULL;

  OpenCLState cl = {NULL};
  cl_mem cl_base = NULL, cl_working = NULL;
  int use_opencl = 0;

  const float fx = 1.0f / tanf(15.0f * (float)M_PI / 180.0f);
  const float fy = fx;
  const float ar = (float)WIDTH / (float)HEIGHT;
  const float fx_over_ar = fx / ar;
  const float near_plane = 0.1f;
  const float far_plane = 1000.0f;
  const float proj_a = far_plane / (far_plane - near_plane);
  const float proj_b = -far_plane * near_plane / (far_plane - near_plane);
  const int count = mesh->vertex_count;

  use_opencl = opencl_init(&cl, "pipeline.cl");
  if (use_opencl) {
    cl_int err;
    cl_base =
        clCreateBuffer(cl.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                       count * sizeof(cl_float4), mesh->vertices, &err);
    if (!cl_base) {
      fprintf(stderr, "Failed to create OpenCL base buffer\n");
      opencl_cleanup(&cl);
      use_opencl = 0;
    }
  }
  if (use_opencl) {
    cl_int err;
    cl_working = clCreateBuffer(cl.context, CL_MEM_READ_WRITE,
                                count * sizeof(cl_float4), NULL, &err);
    if (!cl_working) {
      fprintf(stderr, "Failed to create OpenCL working buffer\n");
      clReleaseMemObject(cl_base);
      opencl_cleanup(&cl);
      use_opencl = 0;
    }
  }

  if (use_opencl) {
    int arg = 0;
    clSetKernelArg(cl.kernel, arg++, sizeof(cl_mem), &cl_base);
    clSetKernelArg(cl.kernel, arg++, sizeof(cl_mem), &cl_working);
    clSetKernelArg(cl.kernel, arg++, sizeof(int), &count);
    arg = 9;
    clSetKernelArg(cl.kernel, arg++, sizeof(float), &cam_x);
    clSetKernelArg(cl.kernel, arg++, sizeof(float), &cam_y);
    clSetKernelArg(cl.kernel, arg++, sizeof(float), &cam_z);
    clSetKernelArg(cl.kernel, arg++, sizeof(float), &fx_over_ar);
    clSetKernelArg(cl.kernel, arg++, sizeof(float), &fy);
    clSetKernelArg(cl.kernel, arg++, sizeof(float), &proj_a);
    clSetKernelArg(cl.kernel, arg++, sizeof(float), &proj_b);
  }

  if (!use_opencl) {
    working = malloc(count * sizeof(Vec4));
    if (!working) {
      free_mesh(mesh);
      return 1;
    }
  }

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    if (cl_working)
      clReleaseMemObject(cl_working);
    if (cl_base)
      clReleaseMemObject(cl_base);
    opencl_cleanup(&cl);
    free(working);
    free_mesh(mesh);
    return 1;
  }

  if (!TTF_Init()) {
    fprintf(stderr, "TTF_Init failed: %s\n", SDL_GetError());
    SDL_Quit();
    if (cl_working)
      clReleaseMemObject(cl_working);
    if (cl_base)
      clReleaseMemObject(cl_base);
    opencl_cleanup(&cl);
    free(working);
    free_mesh(mesh);
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("3D Renderer", WIDTH, HEIGHT, 0);
  if (!window) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    TTF_Quit();
    SDL_Quit();
    if (cl_working)
      clReleaseMemObject(cl_working);
    if (cl_base)
      clReleaseMemObject(cl_base);
    opencl_cleanup(&cl);
    free(working);
    free_mesh(mesh);
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    if (cl_working)
      clReleaseMemObject(cl_working);
    if (cl_base)
      clReleaseMemObject(cl_base);
    opencl_cleanup(&cl);
    free(working);
    free_mesh(mesh);
    return 1;
  }

  TTF_Font *font = TTF_OpenFont("/usr/share/fonts/noto/NotoSans-Bold.ttf", 14);
  if (!font) {
    fprintf(stderr, "TTF_OpenFont failed: %s\n", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    if (cl_working)
      clReleaseMemObject(cl_working);
    if (cl_base)
      clReleaseMemObject(cl_base);
    opencl_cleanup(&cl);
    free(working);
    free_mesh(mesh);
    return 1;
  }

  TransformState state = {0};

  Uint64 prev_ticks = SDL_GetTicks();
  int running = 1;

  Uint64 fps_timer = 0;
  int fps_frame_count = 0;
  float current_fps = 0.0f;
  char fps_text[32] = "";

  while (running) {
    Uint64 now = SDL_GetTicks();
    float dt = (now - prev_ticks) / 1000.0f;
    prev_ticks = now;

    running = process_events(&state, dt);

    if (use_opencl) {
      float rot_x_rad = state.rot_x * ((float)M_PI / 180.0f);
      float rot_y_rad = state.rot_y * ((float)M_PI / 180.0f);
      float rot_z_rad = state.rot_z * ((float)M_PI / 180.0f);

      clSetKernelArg(cl.kernel, 3, sizeof(float), &rot_x_rad);
      clSetKernelArg(cl.kernel, 4, sizeof(float), &rot_y_rad);
      clSetKernelArg(cl.kernel, 5, sizeof(float), &rot_z_rad);
      clSetKernelArg(cl.kernel, 6, sizeof(float), &state.tx);
      clSetKernelArg(cl.kernel, 7, sizeof(float), &state.ty);
      clSetKernelArg(cl.kernel, 8, sizeof(float), &state.tz);

      size_t global = count;
      clEnqueueNDRangeKernel(cl.queue, cl.kernel, 1, NULL, &global, NULL, 0,
                             NULL, NULL);

      cl_int err;
      cl_float4 *mapped =
          clEnqueueMapBuffer(cl.queue, cl_working, CL_TRUE, CL_MAP_READ, 0,
                             count * sizeof(cl_float4), 0, NULL, NULL, &err);

      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderClear(renderer);
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

      Vec4 *verts = (Vec4 *)mapped;
      for (int i = 0; i < mesh->edge_count; i++) {
        Vec4 p1 = verts[mesh->edges[i].a];
        Vec4 p2 = verts[mesh->edges[i].b];
        Vec4 c1, c2;
        if (cohen_sutherland_clip(p1, p2, &c1, &c2))
          draw_edge(renderer, c1, c2);
      }

      clEnqueueUnmapMemObject(cl.queue, cl_working, mapped, 0, NULL, NULL);
    } else {
      memcpy(working, mesh->vertices, count * sizeof(Vec4));

      rotate_x(working, count, state.rot_x);
      rotate_y(working, count, state.rot_y);
      rotate_z(working, count, state.rot_z);
      translate(working, count, state.tx, state.ty, state.tz);
      translate(working, count, -cam_x, -cam_y, -cam_z);
      project(working, count);

      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderClear(renderer);
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

      for (int i = 0; i < mesh->edge_count; i++) {
        Vec4 p1 = working[mesh->edges[i].a];
        Vec4 p2 = working[mesh->edges[i].b];
        Vec4 c1, c2;
        if (cohen_sutherland_clip(p1, p2, &c1, &c2))
          draw_edge(renderer, c1, c2);
      }
    }

    fps_frame_count++;
    if (now - fps_timer >= 1000) {
      current_fps = fps_frame_count * 1000.0f / (now - fps_timer);
      snprintf(fps_text, sizeof(fps_text), "%s FPS: %.0f",
               use_opencl ? "OPENCL" : "CPU", current_fps);
      fps_frame_count = 0;
      fps_timer = now;
    }

    if (fps_text[0]) {
      SDL_Color white = {255, 255, 255, 255};
      SDL_Color black = {0, 0, 0, 255};

      TTF_SetFontOutline(font, 2);
      SDL_Surface *surf = TTF_RenderText_Blended(font, fps_text, 0, white);
      if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        if (tex) {
          SDL_FRect dst = {10.0f, 10.0f, (float)surf->w, (float)surf->h};
          SDL_RenderTexture(renderer, tex, NULL, &dst);
          SDL_DestroyTexture(tex);
        }
        SDL_DestroySurface(surf);
      }

      TTF_SetFontOutline(font, 0);
      surf = TTF_RenderText_Blended(font, fps_text, 0, black);
      if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        if (tex) {
          SDL_FRect dst = {10.0f, 10.0f, (float)surf->w, (float)surf->h};
          SDL_RenderTexture(renderer, tex, NULL, &dst);
          SDL_DestroyTexture(tex);
        }
        SDL_DestroySurface(surf);
      }
    }

    SDL_RenderPresent(renderer);
  }

  if (cl_working)
    clReleaseMemObject(cl_working);
  if (cl_base)
    clReleaseMemObject(cl_base);
  opencl_cleanup(&cl);
  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
  free(working);
  free_mesh(mesh);
  return 0;
}
