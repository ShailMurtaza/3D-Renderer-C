#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clip.h"
#include "events.h"
#include "obj_loader.h"
#include "transformations.h"

static float cam_x = 0.0f, cam_y = 0.0f, cam_z = -150.0f;

static void draw_edge(SDL_Renderer *renderer, Vec4 p1, Vec4 p2) {
  perspective_divide(&p1);
  perspective_divide(&p2);

  float sx1, sy1, sx2, sy2;
  map_to_screen(&p1, &sx1, &sy1);
  map_to_screen(&p2, &sx2, &sy2);

  SDL_RenderLine(renderer, sx1, sy1, sx2, sy2);
}

/* Render a line of text into a texture. Returns NULL on failure. */
static SDL_Texture *make_text(SDL_Renderer *renderer, TTF_Font *font,
                              const char *text, SDL_Color color, float *w,
                              float *h) {
  SDL_Surface *surf = TTF_RenderText_Blended(font, text, 0, color);
  if (!surf) return NULL;

  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
  *w = (float)surf->w;
  *h = (float)surf->h;
  SDL_DestroySurface(surf);
  return tex;
}

static void print_usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s <model.obj> [options]\n"
          "Options:\n"
          "  --record <file.mp4>  Record the live window to an H.264 MP4 via ffmpeg\n"
          "  --fps <F>            Recording frame rate (default 60)\n"
          "  --frames <N>         Stop after N recorded frames (default: until the window is closed)\n",
          prog);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }
  const char *obj_path = argv[1];

  const char *record_path = NULL;
  int export_frames = 0; /* 0 = record until the window is closed */
  int export_fps = 60;

  for (int i = 2; i < argc; i++) {
    if (!strcmp(argv[i], "--record") && i + 1 < argc) {
      record_path = argv[++i];
    } else if (!strcmp(argv[i], "--export") && i + 1 < argc) {
      record_path = argv[++i]; /* alias kept for convenience */
    } else if (!strcmp(argv[i], "--frames") && i + 1 < argc) {
      export_frames = atoi(argv[++i]);
    } else if (!strcmp(argv[i], "--fps") && i + 1 < argc) {
      export_fps = atoi(argv[++i]);
    } else {
      fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
      print_usage(argv[0]);
      return 1;
    }
  }
  if (export_frames < 0) export_frames = 0;
  if (export_fps < 1) export_fps = 1;

  Mesh *mesh = load_obj(obj_path);
  if (!mesh) {
    fprintf(stderr, "Failed to load OBJ file: %s\n", obj_path);
    return 1;
  }

  center_object(mesh->vertices, mesh->vertex_count);

  /* Place the camera just behind the loaded object and pick clip planes that
   * enclose it, so every model is framed consistently regardless of scale. */
  float radius = object_radius(mesh->vertices, mesh->vertex_count);
  float cam_dist, near_plane, far_plane;
  fit_camera(radius, &cam_dist, &near_plane, &far_plane);
  cam_z = -cam_dist;

  Vec4 *working = malloc(mesh->vertex_count * sizeof(Vec4));
  if (!working) {
    free_mesh(mesh);
    return 1;
  }

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    free(working);
    free_mesh(mesh);
    return 1;
  }

  if (!TTF_Init()) {
    fprintf(stderr, "TTF_Init failed: %s\n", SDL_GetError());
    SDL_Quit();
    free(working);
    free_mesh(mesh);
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("3D Renderer", WIDTH, HEIGHT, 0);
  if (!window) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    TTF_Quit();
    SDL_Quit();
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
    free(working);
    free_mesh(mesh);
    return 1;
  }

  /* Optional live recording: capture the actual window contents (including
   * the HUD) every frame and pipe them to ffmpeg. Rendering runs in real time;
   * frames are duplicated when rendering falls behind so the output plays back
   * at a steady `export_fps`, matching the wall-clock duration. */
  FILE *ffmpeg = NULL;
  unsigned char *frame_buf = NULL;
  int out_w = WIDTH, out_h = HEIGHT;
  if (record_path) {
    SDL_GetRenderOutputSize(renderer, &out_w, &out_h);
    if (out_w <= 0 || out_h <= 0) {
      out_w = WIDTH;
      out_h = HEIGHT;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "ffmpeg -y -loglevel error -f rawvideo -pix_fmt rgb24 "
             "-s %dx%d -r %d -i - -an -c:v libx264 -preset medium -crf 18 "
             "-pix_fmt yuv420p '%s'",
             out_w, out_h, export_fps, record_path);
    ffmpeg = popen(cmd, "w");
    if (!ffmpeg) {
      fprintf(stderr, "Failed to start ffmpeg. Is it installed and in PATH?\n");
      TTF_CloseFont(font);
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);
      TTF_Quit();
      SDL_Quit();
      free(working);
      free_mesh(mesh);
      return 1;
    }
    frame_buf = malloc((size_t)out_w * out_h * 3);
    if (!frame_buf) {
      pclose(ffmpeg);
      TTF_CloseFont(font);
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);
      TTF_Quit();
      SDL_Quit();
      free(working);
      free_mesh(mesh);
      return 1;
    }
    fprintf(stderr, "Recording %dx%d at %d fps to %s (close the window to stop)\n",
            out_w, out_h, export_fps, record_path);
  }

  TransformState state = {0};

  Uint64 prev_ticks = SDL_GetTicks();
  int running = 1;

  Uint64 fps_timer = 0;
  int fps_frame_count = 0;
  float current_fps = 0.0f;
  int frame_index = 0;
  Uint64 record_start = prev_ticks;

  const char *model_name = strrchr(obj_path, '/');
  model_name = model_name ? model_name + 1 : obj_path;

  while (running) {
    Uint64 now = SDL_GetTicks();
    float dt = (now - prev_ticks) / 1000.0f; /* real time */
    prev_ticks = now;

    running = process_events(&state, dt);

    memcpy(working, mesh->vertices, mesh->vertex_count * sizeof(Vec4));

    rotate_x(working, mesh->vertex_count, state.rot_x);
    rotate_y(working, mesh->vertex_count, state.rot_y);
    rotate_z(working, mesh->vertex_count, state.rot_z);
    translate(working, mesh->vertex_count, state.tx, state.ty, state.tz);
    translate(working, mesh->vertex_count, -cam_x, -cam_y, -cam_z);
    project(working, mesh->vertex_count, near_plane, far_plane);

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

    fps_frame_count++;
    if (now - fps_timer >= 1000) {
      current_fps = fps_frame_count * 1000.0f / (now - fps_timer);
      fps_frame_count = 0;
      fps_timer = now;
    }

    char hud[8][128];
    int hud_lines = 0;
    snprintf(hud[hud_lines++], sizeof(hud[0]), "FPS: %.1f", current_fps);
    snprintf(hud[hud_lines++], sizeof(hud[0]), "Model: %s", model_name);
    snprintf(hud[hud_lines++], sizeof(hud[0]), "Vertices: %d   Edges: %d",
             mesh->vertex_count, mesh->edge_count);
    snprintf(hud[hud_lines++], sizeof(hud[0]), "Camera dist: %.2f", cam_dist);
    snprintf(hud[hud_lines++], sizeof(hud[0]), "Position: (%.2f, %.2f, %.2f)",
             state.tx, state.ty, state.tz);
    snprintf(hud[hud_lines++], sizeof(hud[0]),
             "Rotation: (%.1f, %.1f, %.1f) deg", state.rot_x, state.rot_y,
             state.rot_z);
    snprintf(hud[hud_lines++], sizeof(hud[0]),
             "WASD/QE move   IJKL/UO rotate   Wheel zoom");

    /* Draw the HUD on a translucent panel so the white text stays readable
     * over the wireframe without needing an outline. */
    SDL_Color white = {255, 255, 255, 255};
    const float pad = 10.0f;
    const float line_h = (float)TTF_GetFontHeight(font) + 2.0f;

    float text_w = 0.0f;
    for (int i = 0; i < hud_lines; i++) {
      int w = 0, h = 0;
      TTF_GetStringSize(font, hud[i], 0, &w, &h);
      if ((float)w > text_w) text_w = (float)w;
    }

    SDL_FRect panel = {8.0f, 8.0f, text_w + pad * 2.0f,
                       (float)hud_lines * line_h + pad * 2.0f};

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);
    SDL_RenderRect(renderer, &panel);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    for (int i = 0; i < hud_lines; i++) {
      float w, h;
      SDL_Texture *tex = make_text(renderer, font, hud[i], white, &w, &h);
      if (!tex) continue;
      SDL_FRect dst = {panel.x + pad, panel.y + pad + (float)i * line_h, w, h};
      SDL_RenderTexture(renderer, tex, NULL, &dst);
      SDL_DestroyTexture(tex);
    }

    if (record_path) {
      /* Grab exactly what is on screen. To keep a steady `export_fps` that
       * matches wall-clock duration, write this frame as many times as needed
       * to catch up with real time (duplicating when we fall behind, dropping
       * extra renders when we run ahead). */
      SDL_Surface *shot = SDL_RenderReadPixels(renderer, NULL);
      if (shot) {
        SDL_ConvertPixels(shot->w, shot->h, shot->format, shot->pixels,
                          shot->pitch, SDL_PIXELFORMAT_RGB24, frame_buf,
                          shot->w * 3);

        long target = (long)((now - record_start) / 1000.0 * export_fps);
        if (frame_index == 0 && target < 1) target = 1; /* start at once */
        while (frame_index < target) {
          fwrite(frame_buf, 1, (size_t)shot->w * shot->h * 3, ffmpeg);
          frame_index++;
          if (export_frames > 0 && frame_index >= export_frames) {
            running = 0;
            break;
          }
        }
        SDL_DestroySurface(shot);
      }
    }

    SDL_RenderPresent(renderer);
  }

  if (ffmpeg) {
    fflush(ffmpeg);
    pclose(ffmpeg);
  }
  free(frame_buf);
  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
  free(working);
  free_mesh(mesh);
  return 0;
}
