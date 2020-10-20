import bpy
import os
import copy

extent_x = 500
num_x = 27
num_y = 27

baseline = extent_x / (num_x - 1) # millimeters

scene = bpy.context.scene

camera = scene.camera

center_pos = copy.deepcopy(camera.location)
initial_seed = copy.deepcopy(scene.cycles.seed);
right, up, back = camera.matrix_world.to_3x3().transposed()

render_filepath = copy.deepcopy(scene.render.filepath)
directory = os.path.dirname(render_filepath)

for j in range(num_y):
    for i in range(num_x):
        u = ((num_x - i) - (num_x + 1) / 2) * baseline;
        v = (j - (num_y - 1) / 2) * baseline;
        camera.location = center_pos + right * u * 1e-3 + up * v * 1e-3;
        
        name = "%s_%02d_%02d_%f_%f_%d_%d" % (camera.name, j, i, -v, u, camera.data.lens, camera.data.sensor_width)
        scene.render.filepath = os.path.join(directory, name)
        
        scene.cycles.seed = i * num_x + j;
        bpy.ops.render.render(write_still=True)

scene.cycles.seed = initial_seed
scene.render.filepath = render_filepath
camera.location = center_pos