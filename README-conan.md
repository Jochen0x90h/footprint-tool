# Build

* Make sure conan 2.x is installed (see below)
* Run $ python cinstall.py in this project to install dependencies to build/ and create CMakeUserPresets.json
* Run $ python install.py in this project to install to ~/.local/bin

# Develop

* Follow build steps until you have CMakeUserPresets.json
* Open an IDE that supports CMakeUserPresets.json (e.g. Visual Studio Code)
* Select configure and build preset for CMake
* Select build target for CMake
* Build

# Configure Conan 2.x

If you use conan for the first time, run

```console
$ conan profile detect
```

It creates a default profile in ~/.conan2/profiles

## Windows
Open default profile (~/.conan2/profiles/default) with text editor, check that the compiler is msvc and set cppstd to 23:

```
[settings]
...
compiler=msvc
compiler.cppstd=23
...
```

If the compiler is missing, install Visual Studio Community Edition (in addition to VSCode!) with C++ for desktop option and run conan profile detect again.

## Linux
Open default profile (~/.conan2/profiles/default) with text editor, check that the compiler is gcc and set cppstd to 23:

```
[settings]
...
compiler=gcc
compiler.cppstd=23
...
```

If you want to let conan install missing packages, add these lines at the end of the profile:

```
[conf]
tools.system.package_manager:mode=install
tools.system.package_manager:sudo=True
```

## Debug Profile

Create debug profile ~/.conan2/profiles/debug by copying the default profile and set

```
build_type=Debug
```

## CMake Presets

Run

```console
$ python cinstall.py
```

It installs dependencies to build/ generates CMakeUserPresets.json which can be used by IDEs such as VSCode
