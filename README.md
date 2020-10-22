# Light Field Renderer
This is a light field renderer that is capable of synthesizing new camera views from a collection of images. 

The renderer is mainly based on the paper [Dynamically Reparameterized Light Fields](https://dash.harvard.edu/handle/1/2634290), but additional features such as a dynamic synthetic aperture and phase detection autofocus has also been implemented.

The program is written in C++ and uses OpenGL for hardware acceleration. The GUI is implemented using [NanoGUI](https://github.com/mitsuba-renderer/nanogui).

## Usage

## Light Fields
I created a separate repository to store a few light fields rendered in [Blender](https://www.blender.org/) using a simple script, [blender-exporter.py](blender-exporter.py). These can be downloaded using:
```
git clone https://github.com/linusmossberg/light-fields
```
The renderer is also capable of rendering rectified light fields from the Stanford Lego Gantry, which can be downloaded here: http://lightfield.stanford.edu/lfs.html.

### Config
The start values and ranges of sliders and such in the GUI can be modified by adding a `config.cfg` file to the light field folder that specifies the start, min and max value for different properties: 
```
# [property name] [start value] [min value] [max value]
focal-length 50 10 100
sensor-size 35 10 100
focus-distance 16 10 50
f-stop 1.8 0.1 5.6
```
Unspecified properties uses the default values. All property names are available in [config.cpp](source/core/config.cpp#L44).

## Building
The program uses [CMake](https://cmake.org/) and it can be cloned and built using:
<pre><code>git clone <b>--recursive</b> https://github.com/linusmossberg/light-field-renderer
cd light-field-renderer
mkdir build
cd build
cmake ..</code></pre>
This will create build files for your platform that should work automatically, but a few additional packages are required on certain platforms. On Debian/Ubuntu, these can be installed using:
```
apt-get install xorg-dev libglu1-mesa-dev
```
and on Fedora:
```
sudo dnf install mesa-libGLU-devel libXi-devel libXcursor-devel libXinerama-devel libXrandr-devel xorg-x11-server-devel
```