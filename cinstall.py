# Calls "conan install" for all presets in cpresets.txt
# Also creates a CMakeUserPresets.json which is supported by IDEs such as VSCode
#
# usage:
# 1: Copy cpresets.txt containing a list of presets from support/conan/[operating system] to project root (next to this file)
# 2: Optional: Open cpresets.txt in an editor and adjust to own needs
# 3: $ python cinstall.py
#

import platform
import json
from pathlib import Path
import shlex
import subprocess


# configuration
install_prefix = str(Path.home() / ".local")

# get system (Linux, Darwin, Windows)
system = platform.system()
#print(f"system {system}")

# read presets from presets.txt
file = open('cpresets.txt', 'r')
presets = file.readlines()
file.close()

# cmake presets
cmakePresets = {
    "version": 3,
    "configurePresets": [],
    "buildPresets": [],
    "testPresets": []
}

# add a preset to the cmake presets
def addPreset(type, name):
    cmakePresets[type].append(
        {
            "name": name,
            "configurePreset": name
        }
    )

def addPresetWithBuildType(type, name, build_type):
    cmakePresets[type].append(
        {
            "name": name,
            "configurePreset": name,
            "configuration": build_type
        }
    )

# iterate over presets
for preset in presets:
    p = shlex.split(preset)
    if preset.startswith('#') or len(p) < 2:
       continue

    # check optional system
    if len(p) >= 3 and p[2] != system:
       continue

    profile = p[0]
    generator = p[1]

    # get build_type (Debug/Release) from profile
    result = subprocess.run(f"conan profile show -pr:h={profile} --format=json", shell=True, capture_output=True, check=True)
    j = json.loads(result.stdout)
    build_type = j.get("host", {}).get("settings", {}).get("build_type")
    if build_type is None:
        print(f"Warning: build type for profile {profile} not found")
        continue

    if build_type == 'Release':
        name = system
    else:
        name = f"{system}-{build_type}"

    # install dependencies using conan
    print(f"*** Installing dependencies for profile {profile} ({build_type}) ***")
    subprocess.run(f"conan install -pr:b default -pr:h {profile} -b missing -of build/{name} .", shell=True)

    # create cmake presets
    cmakePresets["configurePresets"].append(
        {
            "name": name,
            "description": f"({generator})",
            "generator": generator,
            "cacheVariables": {
                #"CMAKE_POLICY_DEFAULT_CMP0077": "NEW",
                "CMAKE_POLICY_DEFAULT_CMP0091": "NEW",
                "CMAKE_BUILD_TYPE": build_type,
                "CMAKE_INSTALL_PREFIX": install_prefix
            },
            #"toolchainFile": str(os.getcwd() / f"build/{name}/conan_toolchain.cmake"),
            "toolchainFile": f"build/{name}/conan_toolchain.cmake",
            "binaryDir": f"build/{name}"
        }
    )
    if "Visual Studio" in generator:
        addPresetWithBuildType("buildPresets", name, build_type)
        addPresetWithBuildType("testPresets", name, build_type)
    else:
        addPreset("buildPresets", name)
        addPreset("testPresets", name)

# save cmake presets to CMakeUserPresets.json
file = open("CMakeUserPresets.json", "w")
file.write(json.dumps(cmakePresets, indent=4))
file.close()
