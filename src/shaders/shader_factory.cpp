/* This file is included by orh_d3d11.cpp. It includes and creates shaders.

Use like this:

#include "shaders/mesh_phong.cpp"
#include "shaders/skeletal_mesh_pbr.cpp"
...
...
#include "shaders/sprite_instanced.cpp"

Then in shader_factory_create_shaders(), you do:
mesh_phong_create_shader();
skeletal_mesh_pbr_create_shader();
...
...
sprite_instanced_create_shader();


NOTE:
* This is just to organize stuff. Each shader will have its own utility functions as to how to bind
 and update its buffers and how it sends data to GPU, etc. No specific naming conversion or high level
"Shader" structure/class... user makes the functions he cares about in shaders, and simply calls function
 responsible for compiling the shader in shader_factory_create_shaders(), that's it.

*/

// Include shaders here.
#include "shaders/skeletal_mesh_pbr.cpp"

FUNCTION void shader_factory_create_shaders()
{
    // Call the "create function" for the shaders here.
    skeletal_mesh_pbr_create_shader();
}