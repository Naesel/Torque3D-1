# CEF T3D Plugin Installation Instructions

Download and extract the CEF 'Standard Distribution' for your target platform from: (https://cef-builds.spotifycdn.com/index.html). The most recently tested build is cef 90.6.7 + chromium-90.0.4430.212, stable binary May 27, 2021.

Clone [Torque3D](https://github.com/TorqueGameEngines/Torque3D).

Pull https://github.com/OTHGMars/Torque3D-1/tree/CEFPlugin into your local branch.

## Windows:
1. Build the cef wrapper binary. (This step is only needed to build the dll wrapper. All other required binaries are provided in the distribution).
   * Configure cef for static linking to T3D.
     * Under the folder you extracted cef to, open *cmake/cef_variables.cmake* in an editor.
     * Remove (or comment out) the following (lines 494-496 in 90.6.7):
```
list(APPEND CEF_COMPILER_DEFINES_DEBUG
      _HAS_ITERATOR_DEBUGGING=0   # Disable iterator debugging
      )
```
     * Save and close the file.
   * Use cmake to open CMakeLists.txt in the folder you extracted cef to. Set output to the same directory.
   * Configure and generate the cef project for the same VS version and platform that you will be using to build T3D.
   * Open cef.sln in VS and create Release and Debug builds of the libcef_dll_wrapper project.

2. Build Torque3D.
   * Open your t3d cmake project. Set TORQUE_CEF_WEB on. Set TORQUE_CEF_PATH to the directory where you extracted the binary distribution. Set TorqueTemplate to 'BaseGame'.
   * Configure and generate your project.
   * Build the solution in VS. Be sure to also build the INSTALL project, as it will copy the binaries and resources needed by cef into your game directory.

3. Start using html guis. The install scripts copied the webCtrlDemo Module to your game/data directory. The demo provides gui based examples of interaction with the GuiWebCtrl and WebURLRequest objects.


## Linux
Coming Soon...
