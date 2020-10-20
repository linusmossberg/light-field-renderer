import bpy
import os
import copy

scene = bpy.context.scene

camera = scene.camera

center_pos = copy.deepcopy(camera.location)
initial_seed = copy.deepcopy(scene.cycles.seed);
right, up, back = camera.matrix_world.to_3x3().transposed()

extent_x = 4000
num_x = 101
num_y = 63

baseline = extent_x / (num_x - 1) # millimeters

for j in range(num_y):
    for i in range(num_x):
        u = ((num_x - i) - (num_x + 1) / 2) * baseline;
        v = (j - (num_y - 1) / 2) * baseline;
        camera.location = center_pos + right * u * 1e-3 + up * v * 1e-3;
        
        name = "%s_%02d_%02d_%d_%d_%d_%d" % (camera.name, j, i, -v, u, camera.data.lens, camera.data.sensor_width)
        scene.render.filepath = os.path.join("C:/tmp", name)
        
        scene.cycles.seed = i * num_x + j;
        bpy.ops.render.render(write_still=True)

scene.cycles.seed = initial_seed
camera.location = center_pos