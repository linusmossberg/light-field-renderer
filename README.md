# Light Field Renderer

This is a light field renderer that can synthesize new camera views from a collection of images.

<img src="https://user-images.githubusercontent.com/15798094/97102178-bb224e00-16a3-11eb-827a-88afd72a8f8d.gif" width="100%"/>

The renderer is mainly based on the paper [Dynamically Reparameterized Light Fields](https://dash.harvard.edu/handle/1/2634290), but additional features such as a dynamic synthetic aperture and phase detection autofocus has also been implemented. A report describing the renderer in more detail is available [here](report.pdf).

The program is written in C++ and uses OpenGL for hardware acceleration. The GUI is implemented using [NanoGUI](https://github.com/mitsuba-renderer/nanogui).

## Usage

![Screenshot](https://user-images.githubusercontent.com/15798094/97706716-683b0300-1ab6-11eb-9ac0-67f66a4f3765.png)

The image above is a screenshot of the program. Most aspects of the camera can be changed, such as position, rotation, focal length, depth of field (f-stop), focus distance etc. Most settings are hopefully self-explanatory, and some have explanatory tooltips. 

### Controls

| Key                   | Action                |
|-----------------------|-----------------------|
| `W`                   | Move forward          |
| `A`                   | Move left             |
| `S`                   | Move back             |
| `D`                   | Move right            |
| `SPACE`               | Move up               |
| `CTRL`                | Move down             |
| `SCROLL`              | Change focus distance |
| `SHIFT`+`MOUSE CLICK` | Autofocus             |

Clicking and holding down a mouse button in the render view activates mouse navigation, which is deactivated once the button is released. Mouse navigation is used to rotate the camera in `Free` mode (like in a first-person game) and to move the camera laterally in `Target` mode.

## Examples

### Dolly Zoom
<img src="https://user-images.githubusercontent.com/15798094/97700126-97984280-1aab-11eb-87ea-e0dbcea23b9c.gif"/>
Moving the camera back and forth while changing the focal length. This clearly shows that the synthesized view is a combination of several (hundred) images since all input images have the same focal length and are located on a plane perpendicular to the movement direction.

### Focus Change
<img src="https://user-images.githubusercontent.com/15798094/97703913-b8639680-1ab1-11eb-8d8d-947ec173f4b5.gif"/>
Moving the focal plane between the shop owner and the metal sphere while using a large aperture.

## Light Fields

The included light field was rendered in [Blender](https://www.blender.org/) using the simple script [blender-exporter.py](blender-exporter.py). The scene was created by [Alex Treviño](http://www.aendom.com/) using concept by [Anaïs Maamar](https://www.artstation.com/chatonlaser).

The renderer is also capable of rendering rectified light fields from the Stanford Lego Gantry, which can be downloaded here: http://lightfield.stanford.edu/lfs.html.

### Config

The start values and ranges of sliders and such in the GUI can be modified by adding a `config.cfg` file to the light field folder that specifies the start, min and max value for different properties: 
```cfg
# [property name] [start value] [min value] [max value]
focal-length 50 10 100
focus-distance 16 10 50
f-stop 1.8 0.1 5.6
```
Unspecified properties uses the default values. All property names are available in [config.cpp](source/core/config.cpp#L59).

## Building

Start by cloning the program and all submodules using the following command:
<pre><code>git clone <b>--recursive</b> https://github.com/linusmossberg/light-field-renderer</code></pre>
Build files can then be generated with [CMake](https://cmake.org/) using:
```sh
cd light-field-renderer
mkdir build
cd build
cmake ..
```
These should work automatically, but a few additional packages are required on certain platforms. On Debian/Ubuntu, these can be installed using:
```sh
apt-get install xorg-dev libglu1-mesa-dev
```
and on RedHat/Fedora:
```sh
sudo dnf install mesa-libGLU-devel libXi-devel libXcursor-devel libXinerama-devel libXrandr-devel xorg-x11-server-devel
```
