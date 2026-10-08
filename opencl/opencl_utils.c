#include "opencl_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "Failed to open %s\n", path);
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *src = malloc(len + 1);
  if (!src) {
    fclose(f);
    return NULL;
  }
  if (fread(src, 1, len, f) != (size_t)len) {
    free(src);
    fclose(f);
    return NULL;
  }
  src[len] = '\0';
  fclose(f);
  return src;
}

int opencl_init(OpenCLState *cl, const char *source_path) {
  cl_int err;
  cl_platform_id platforms[16];
  cl_uint platform_count;

  err = clGetPlatformIDs(16, platforms, &platform_count);
  if (err != CL_SUCCESS || platform_count == 0) {
    fprintf(stderr, "No OpenCL platforms found\n");
    return 0;
  }

  cl_bool device_found = CL_FALSE;

  // Step 1: Look for a GPU across ALL available platforms
  for (cl_uint i = 0; i < platform_count; i++) {
    err =
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 1, &cl->device, NULL);
    if (err == CL_SUCCESS) {
      cl->platform = platforms[i];
      device_found = CL_TRUE;
      break; // Found a valid GPU platform, stop looking
    }
  }

  // Step 2: Fallback to a CPU device only if no GPU was found anywhere
  if (!device_found) {
    for (cl_uint i = 0; i < platform_count; i++) {
      err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_CPU, 1, &cl->device,
                           NULL);
      if (err == CL_SUCCESS) {
        cl->platform = platforms[i];
        device_found = CL_TRUE;
        break;
      }
    }
  }

  if (!device_found) {
    fprintf(stderr,
            "OpenCL platform found but no devices available.\n"
            "Tip: if using Mesa's RustiCL, set RUSTICL_ENABLE=radeonsi\n"
            "  e.g.: RUSTICL_ENABLE=radeonsi ./renderer\n"
            "Or install a different OpenCL implementation:\n"
            "  sudo emerge sci-libs/pocl        (CPU OpenCL)\n"
            "  sudo emerge dev-libs/rocm-opencl-runtime  (AMD GPU)\n");
    return 0;
  }

  cl->context = clCreateContext(NULL, 1, &cl->device, NULL, NULL, &err);
  if (!cl->context) {
    fprintf(stderr, "Failed to create OpenCL context\n");
    return 0;
  }

  cl->queue =
      clCreateCommandQueueWithProperties(cl->context, cl->device, NULL, &err);
  if (!cl->queue) {
    fprintf(stderr, "Failed to create command queue\n");
    clReleaseContext(cl->context);
    return 0;
  }

  char *src = read_source(source_path);
  if (!src) {
    clReleaseCommandQueue(cl->queue);
    clReleaseContext(cl->context);
    return 0;
  }

  cl->program = clCreateProgramWithSource(cl->context, 1, (const char **)&src,
                                          NULL, &err);
  free(src);
  if (!cl->program) {
    fprintf(stderr, "Failed to create OpenCL program\n");
    clReleaseCommandQueue(cl->queue);
    clReleaseContext(cl->context);
    return 0;
  }

  err = clBuildProgram(cl->program, 1, &cl->device, NULL, NULL, NULL);
  if (err != CL_SUCCESS) {
    char log[4096];
    clGetProgramBuildInfo(cl->program, cl->device, CL_PROGRAM_BUILD_LOG,
                          sizeof(log), log, NULL);
    fprintf(stderr, "OpenCL build log:\n%s\n", log);
    clReleaseProgram(cl->program);
    clReleaseCommandQueue(cl->queue);
    clReleaseContext(cl->context);
    return 0;
  }

  cl->kernel = clCreateKernel(cl->program, "transform_vertices", &err);
  if (!cl->kernel) {
    fprintf(stderr, "Failed to create kernel\n");
    clReleaseProgram(cl->program);
    clReleaseCommandQueue(cl->queue);
    clReleaseContext(cl->context);
    return 0;
  }

  return 1;
}

void opencl_cleanup(OpenCLState *cl) {
  if (!cl)
    return;
  if (cl->kernel)
    clReleaseKernel(cl->kernel);
  if (cl->program)
    clReleaseProgram(cl->program);
  if (cl->queue)
    clReleaseCommandQueue(cl->queue);
  if (cl->context)
    clReleaseContext(cl->context);
}
