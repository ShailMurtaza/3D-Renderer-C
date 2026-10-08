__kernel void transform_vertices(
    __global const float4* restrict in_vertices,
    __global float4* restrict out_vertices,
    const int count,
    const float rot_x_rad,
    const float rot_y_rad,
    const float rot_z_rad,
    const float tx,
    const float ty,
    const float tz,
    const float cam_x,
    const float cam_y,
    const float cam_z,
    const float fx_over_ar,
    const float fy,
    const float proj_a,
    const float proj_b
) {
    int i = get_global_id(0);
    if (i >= count) return;

    float4 v = in_vertices[i];

    float cx = cos(rot_x_rad), sx = sin(rot_x_rad);
    float y = v.y * cx - v.z * sx;
    float z = v.y * sx + v.z * cx;
    v.y = y; v.z = z;

    float cy = cos(rot_y_rad), sy = sin(rot_y_rad);
    float x = v.x * cy - v.z * sy;
    z = v.x * sy + v.z * cy;
    v.x = x; v.z = z;

    float cz = cos(rot_z_rad), sz = sin(rot_z_rad);
    x = v.x * cz - v.y * sz;
    y = v.x * sz + v.y * cz;
    v.x = x; v.y = y;

    v.x += (tx - cam_x) * v.w;
    v.y += (ty - cam_y) * v.w;
    v.z += (tz - cam_z) * v.w;

    float w_save = v.z;
    v.x = v.x * fx_over_ar;
    v.y = v.y * fy;
    v.z = v.z * proj_a + v.w * proj_b;
    v.w = w_save;

    out_vertices[i] = v;
}
