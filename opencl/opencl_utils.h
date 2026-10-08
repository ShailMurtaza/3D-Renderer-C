#ifndef OPENCL_UTILS_H
#define OPENCL_UTILS_H

#include <CL/cl.h>

typedef struct {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
} OpenCLState;

int opencl_init(OpenCLState *cl, const char *source_path);
void opencl_cleanup(OpenCLState *cl);

#endif
