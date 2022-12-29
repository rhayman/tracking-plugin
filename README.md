# Tracking Plugin

Uses 2D position data from Bonsai to deliver closed-loop stimulation.

The original implementation of this plugin is described in Buccino et al. (2018) ["Open source modules for tracking animal behavior and closed-loop stimulation based on Open Ephys and Bonsai"](http://iopscience.iop.org/article/10.1088/1741-2552/aacf45/meta) *J Neural Eng* **15** 055002.

## Installation

This plugin can be added via the Open Ephys GUI's built-in Plugin Installer. Press **ctrl-P** or **⌘P** to open the Plugin Installer, browse to the "Tracking Plugin", and click the "Install" button. The Tracking Plugin should now be available to use.

## Usage

Documentation related to the Tracking Plugin user interface and the required Bonsai configuration are available [here](https://open-ephys.github.io/gui-docs/User-Manual/Plugins/Tracking-Plugin.html).

**Important:** This plugin has been substantially updated for GUI version `0.6.x`. Most notably, all of the functionality is now encapsulated into a single "Filter" plugin, rather than a Source, Filter, and Sink. If you've been using this plugin previously, please read through the documentation carefully to understand what has changed.


## Building from source

First, follow the instructions on [this page](https://open-ephys.github.io/gui-docs/Developer-Guide/Compiling-the-GUI.html) to build the Open Ephys GUI.

**Important:** This plugin is intended for use with the latest version of the GUI, `0.6.x`. For a version that works with `0.5.x`, please use the code on the `archive` branch.

Then, clone this repository into a directory at the same level as the `plugin-GUI`, e.g.:
 
```
Code
├── plugin-GUI
│   ├── Build
│   ├── Source
│   └── ...
├── OEPlugins
│   └── tracking-plugin
│       ├── Build
│       ├── Source
│       └── ...
```

### Windows

**Requirements:** [Visual Studio](https://visualstudio.microsoft.com/) and [CMake](https://cmake.org/install/)

From the `Build` directory, enter:

```bash
cmake -G "Visual Studio 17 2022" -A x64 ..
```

Next, launch Visual Studio and open the `OE_PLUGIN_tracking-plugin.sln` file that was just created. Select the appropriate configuration (Debug/Release) and build the solution.

Selecting the `INSTALL` project and manually building it will copy the `.dll` and any other required files into the GUI's `plugins` directory. The next time you launch the GUI from Visual Studio, the Tracking Plugin should be available.


### Linux

**Requirements:** [CMake](https://cmake.org/install/)

From the `Build` directory, enter:

```bash
cmake -G "Unix Makefiles" ..
cd Debug
make -j
make install
```

This will build the plugin and copy the `.so` file into the GUI's `plugins` directory. The next time you launch the compiled version of the GUI, the Tracking Plugin should be available.


### macOS

**Requirements:** [Xcode](https://developer.apple.com/xcode/) and [CMake](https://cmake.org/install/)

From the `Build` directory, enter:

```bash
cmake -G "Xcode" ..
```

Next, launch Xcode and open the `tracking-plugin.xcodeproj` file that now lives in the “Build” directory.

Running the `ALL_BUILD` scheme will compile the plugin; running the `INSTALL` scheme will install the `.bundle` file to `/Users/<username>/Library/Application Support/open-ephys/plugins-api`. The Tracking Plugin should be available the next time you launch the GUI from Xcode.