import bpy
import os
import copy

# Usage:
#
# 1. Set up your scene, camera and render settings in Blender as usual
# 2. Open the Text Editor in Blender and open this file (blender-exporter.py)
# 3. Change the following variables depending on the desired size of the light field:
#
extent_x = 800 # extent of camera plane in horizontal direction in millimeters
num_x    = 27  # number of cameras in horizontal direction
num_y    = 27  # number of cameras in vertical direction
#
# 4. Execute the script by clicking the run button
#
# This will freeze blender until all of the images have been rendered. Images are saved 
# to the directory and in the format chosen in the output settings, and they are rendered 
# using the chosen render settings and the camera settings of the active camera. 
# The active camera is used as the center of the camera plane.

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
        x = ((num_x - i) - (num_x + 1) / 2) * baseline;
        y = (j - (num_y - 1) / 2) * baseline;
        camera.location = center_pos + right * x * 1e-3 + up * y * 1e-3;
        
        name = "%s_%02d_%02d_%f_%f_%f_%f" % (camera.name, j, i, -y, x, camera.data.lens, camera.data.sensor_width)
        scene.render.filepath = os.path.join(directory, name)
        
        scene.cycles.seed = i * num_x + j;
        bpy.ops.render.render(write_still=True)

scene.cycles.seed = initial_seed
scene.render.filepath = render_filepath
camera.location = center_pos