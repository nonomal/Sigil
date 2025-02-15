# <center>Building Sigil on Windows with Qt6</center>

## General Overview

To build Sigil on Windows, you need to get/do the following things:

1. [Visual Studio 2022+](#vsstudio) The free Community Edition will work fine
2. [CMake](#cmake) (3.18 or higher)
3. [Inno Setup](#inno) (Version 6.3.0 - unicode version - or higher required)
4. [Qt6.8.2+/QtWebEngine](#qt6)
5. [Python 3.13.2+](#python)
6. [The Sigil source code](#sigil) (downloaded zipfile or a git clone)
7. [Building Sigil](#build)
8. [Advanced stuff](#advanced)

## <a name="vsstudio"/>Visual Studio

Sigil for Qt6 is built with VS2022 in the latest versions of Sigil. Begin with making sure you have a working version of [Visual Studio](https://visualstudio.microsoft.com/vs/community/), (the free Community edition will work fine). Older versions of  Visual Studio can be found [here](https://visualstudio.microsoft.com/vs/older-downloads/)

The instructions given here will focus on using the command-line cmake and nmake tools. But if you're more comfortable in an IDE, you should find sufficient instructions to get you going. I simply don't use the IDE. Too many fiddly bits (sign-ins and expiring licenses for free software) for my taste. But it did work the last time I tried it.

From the Start button (you're on your own if you don't have one), go to "All Programs->Visual Studio 2022" and find the command prompt you'll need for your platform. Probably "VS2022 x64 Native Tools Command Prompt" for building a 64-bit package. Create a shortcut to the applicable command-prompt on your Desktop. That's what you'll be using to configure and build Sigil.

If you're going to use the Visual Studio IDE and cmake-gui, you won't need to use these command-prompts.

## <a name="cmake"/>Getting CMake
CMake 3.18 or better is required. Download it from [cmake.org](http://www.cmake.org) and install it. **Make sure CMake's "bin" directory (the one containing "cmake.exe") is added to your PATH**.

## <a name="inno"/>Inno Setup
Get the unicode version of version 6.3.0+ from [jrsoftware.org](http://www.jrsoftware.org/isdl.php) make sure you say yes to the Preprocessor option when installing. **Also make sure the Inno Setup directory (the one containing "ISCC.exe") is added to your PATH**. There is no 64-bit version of Inno Setup, but you can still use it to create 64-bit program installers.

## <a name="qt6"/>Qt6.8.2+
As of Qt6+, there there are no binary Qt installers provided for free. You must compile it yourself from source (applying the appropriate patches from the Sigil Docs directory). The official Windows installer is currently using Qt6.8.2.

If you want to use the exact, patched versions of Qt6.8.2 and QtWebEngine that I'm using to release Sigil, feel free to download the whole shebang [from my personal repository](https://github.com/dougmassay/win-qtwebkit-5.212/releases/tag/v5.212-1). It's the archive named: Qt6.8.2ci_x64_VS2022.7z. Unzip it with 7-zip and note the location.

Once you have Qt6.8.2+ built/installed/unzipped, **make sure its "bin" directory (the one containing "windeployqt.exe) is added to your PATH**

## <a name="python"/>Getting Python 3.13.2+
**This is important**. You're going to be building the 64-bit version of Sigil, so you need to install the 64-bit version of Python 3.11.9+. 

The official Windows Sigil installer uses Python 3.13.2 from [Python.org](http://www.python.org).  Other flavors of Python may work, but you're on your own if they don't. Download it and install it. If you install somewhere that requires special privileges to add/remove files, you may need to use an administator command prompt to install Sigil's extra Python module dependencies. **I recommend installing Python to the default location ($USER/appdata) to avoid that problem. I also recommend allowing the Python installer to add Python to your PATH**. This will make it easier for Sigil to locate the necessary Python pieces it needs, and will make it easy to install the extra Python modules using Pythons "pip" tool. I'm going to assume you've done so for the rest of these instructions.

### Getting the extra Python module dependencies
After installing Python 3.13.2+, I recommend making sure Python's pip/setuptools is updated to the latest version. The easiest way to do this is to open a command prompt (the shortcut to the Visual Studio command prompt you made on your desktop [in step 1](#vsstudio) will work fine) and type:

>`python -m pip install -U pip`

Once finished, you can begin to install the extra modules needed by Sigil.

+ six
+ html5lib (1.1+)
+ regex (2024.11.6+)
+ cssselect (1.2.0+)
+ css-parser (1.0.10+)
+ chardet (5.2.0+)
+ dulwich (0.22.7+) dulwich also requires urllib3 and certifi minimums
+ Pillow (11.1.0+)
+ lxml (5.3.0+)
+ Shiboken6 (6.8.2+)
+ PySide6 (6.8.2+) Use the same version as Shiboken6
+ Pyside6-Addons (6.8.2+)
(I've also compiled PySide/Shiboken6 for Qt6.8.2. You can download the wheels from the same place as my custom version of Qt6.8.2)

From the same command prompt you updated pip with, install the "six" module with the following command:

>`pip install six`

Repeat for the next six modules:

>`pip install html5lib`

etc...

### Installing Pillow

Other versions of Pillow will probably work fine, but Sigil's installer build is predicated on a v11.1.0 minimum. To install that specific version, use the following pip command.

>`pip install Pillow==11.1.0`

Otherwise:

> `pip install Pillow`

will suffice.

### Installing lxml.

Install a specific version with pip using the following command

>`pip install lxml==5.3.0`

### Installing PySide6.

PyQt5 was replaced with Qt's own Shiboken/PySide6 with Sigil-Qt6

>`pip install PySide6==6.8.2 Shiboken6==6.8.2 PySide6-Addons==6.8.2`

PySide6-Addons provides WebEngine and PDF support

## <a name="sigil"/>Getting Sigil's Source Code

You can clone the Sigil Github repository (Requires a Windows git client - I use the [portable version from here](https://github.com/git-for-windows/git/releases/latest)):

>`git clone https://github.com/Sigil-Ebook/Sigil.git`

Or you can download a specific release's zipfile from Sigil's [releases page](https://github.com/Sigil-Ebook/Sigil/releases/latest) on Github (2.3.0 at the time of this writing).

I recommend the latter method, as the github repository version might not always be stable at any given moment (even though we try hard not to leave it broken).

Unzip the source code. Rename the uppermost directory to something useful like "sigil-src". Unless you like typing extra-long directory names in command-prompts--in which case, don't rename it. Remember this location, you'll need it when generating the nmake makefiles with cmake

### Preparing Sigil's Source Code

To build the Sigil installer package, you'll need to copy the Visual Studio redistributable runtime installer to the `<sigil-src>\installer` folder (the one that contains the Sigil.iss file). These redistributable files can usually be found somewhere in Visual Studio's folder structure:

`C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.XX.XXXXX\`

vc_redist.x64.exe for 64-bit builds.

**The file names are important so don't rename them**. Just copy the appropriate one to the "installer" folder in Sigil's source mentioned above.

## <a name="build"/>Configuring and building Sigil (and the Sigil installer package)

### Configuring Sigil with cmake

With all the pre-requisites met and all the necessary additions to your PATH, the only thing left to do is generate the Sigil makefiles with cmake.

Using the shortcut to the proper VSStudio command-prompt created in [step 1](#vsstudio), cd to a suitable empty directory for building Sigil (I recommend "sigil-build", or some such similar name), and issue the following command:

> `cmake -G "NMake Makefiles" -DWIN_INSTALLER_USE_64BIT_CRT=1 -DQt6_DIR="C:\Qt6.8.2\lib\cmake\Qt6" -DCMAKE_BUILD_TYPE=Release "C:\path\to\sigil-src"`

Obviously change the paths to match where you've actually installed Qt6.8.x+ and the Sigil source code. For instance: using my specially compiled version of Qt6/WebEngine, it would look like:

`cmake -G "NMake Makefiles" -DWIN_INSTALLER_USE_64BIT_CRT=1 -DQt6_DIR="C:Qt6.8.2\lib\cmake\Qt6" -DCMAKE_BUILD_TYPE=Release "C:\path\to\sigil-src"`

**Starting with Sigil 2.0.2, the -DUSE_QT6=1 is assumed and no longer used. If you wish to build Sigil with Qt5, you will need to add -DUSE_QT5=1 to the cmake configure command. Even this will be unavailable starting with Sigil 2.3.0**

If this completes successfully, then you're ready to compile Sigil (leave the command prompt open).

You can also generate Visual Studio Project/Solution Files with cmake by using:

> `cmake -G "Visual Studio 17 2022" -A x64 -DWIN_INSTALLER_USE_64BIT_CRT=1 -DQt6_DIR="C:\Qt6.8.2\lib\cmake\Qt6" -DCMAKE_BUILD_TYPE=Release "C:\path\to\sigil-src"`


You can also use cmake-gui (double-click on cmake-gui in the cmake/bin directory) and avoid using the command-prompt altogether if you wish (although you're on your own in figuring out how to enter all the cmake configuration options in the gui).

#### Advanced CMAKE Options

-DUSE_QT5=(0|1) Defaults to 0 (Use Qt6). Build Sigil using Qt6 or Qt5 (NOTE: this will be ineffective starting with Sigil 2.3.0)

The following three cmake options are used to manually specify which Python3 you want to use when building Sigil instead of relying on the included cmake utilities to try and automatically find a suitable version. They can come in handy if you have multiple versions of Python 3 installed on your computer.

-DPYTHON_LIBRARY=`<the full path to the python3.x library (ex. python313.lib)>`

-DPYTHON_INCLUDE_DIR=`<the path to the directory where python3.x's header files (python.h) can be found>`

-DPYTHON_EXECUTABLE=`<the full path to the python3.x binary (python.exe)>`

**NOTE:** those using cmake 3.18.1 or higher can use the newer FindPython3 cmake module for locating the pieces of Python3 that Sigil needs to compile. The build process defaults to the old FindPython. Add -DTRY_NEWER_FINDPYTHON3=1 to your CMAKE configure command to use it. Newer versions of the above  options to specify a particular version of Python3 (in case you have multiple Pythons installed) would be as follows:

-DPython3_LIBRARIES=`<the full path to the python3.x library (ex. python313.lib)>`

-DPython3_INCLUDE_DIRS=`<the path to the directory where python3.x's header files (python.h) can be found>`

-DPython3_EXECUTABLE=`<the full path to the python3.x binary (python.exe)>`


If you don't want to build/include the bundled Python environment in the Sigil installer, use the -DPKG_SYSTEM_PYTHON=0 in the CMake configure command to disable it. **NOTE**: you'll have to configure an external Python interpeter for running Sigil plugins. The "Use Bundled Python" feature will be unavailable.

### Compiling Sigil

If you generated NMake Makefiles with cmake (like I do), then compile Sigil by typing `nmake` (at the same command-prompt you just configured with) to begin building Sigil. If it completes without error, you're ready to build the installer package (leave the command prompt open).

If you generated Visual Studio projects/solutions, then open the Sigil.sln file in the build directory; make sure the solution configuration is set to "Release"; select the ALL_BUILD project in the Solution Explorer and build the ALL_BUILD project from the Build menu (Build->Build ALL_BUILD). **Note: don't build the solution**. If it completes without error, you're ready to build the installer package. It is not possible at this time build a Debug version of Sigil with Visual Studio. So make sure the solution configuration is changed to build "Release" only.

### Building the Sigil installer package

If you generated NMake Makefiles and have successfully compiled Sigil, then type `nmake makeinstaller` (at the same command prompt you just compiled Sigil with) to build the Sigil installer package. If it completes succesfully, the Sigil installer will be placed in the sigil-build directory's "installer" folder (NOTE: that's the *build* directory and not the *source* directory). If it doesn't complete succesfully, you may have to delete the "temp_folder" in the build directory before proceeding.

If you generated Visual Studio project/solutions and have built the ALL_BUILD project successfully, then select the "makeinstaller" project in the Solution Explorer and build the makeinstaller project from the Build menu (Build->Build makeinstaller). If it completes succesfully, the Sigil installer will be placed in the sigil-build directory's "installer" folder (NOTE: that's the *build* directory and not the *source* directory). If it doesn't complete succesfully, you may have to delete the "temp_folder" in the build directory before proceeding.

## <a name="advanced"/>Advanced

### Environment Variables

The following are environment variables that can be set at runtime to affect how Sigil is run after building/installing.

SIGIL_PREFS_DIR - Changes where sigil looks for and updates its user preference data. Needs to specify a full path in a directory where the user has write privileges.

SKIP_SIGIL_UPDATE_CHECK - Defining this variable (to any value) will cause Sigil to skip its online check for a newer version.
