import os
import json
from pathlib import Path


# configuration
home = Path.home()
vcpkg = Path(os.environ['VCPKG_ROOT'])
generator = "Visual Studio 17 2022"
triplet = "x64-windows-static"


# template for cmake presets
presets = {
	"version": 3,
	"configurePresets": [],
	"buildPresets": [],
	"testPresets": []
}
def addPreset(type, triplet, configuration):
	presets[type].append(
		{
			"name": f"{triplet}-{configuration}",
			"configurePreset": triplet,
			"configuration":configuration
		}
	)

# install dependencies
os.system(f"vcpkg install --triplet {triplet} --x-install-root vcpkg/{triplet}")

# create cmake presets
presets["configurePresets"].append(
	{
		"name": triplet,
		"description": f"({generator})",
		"generator": generator,
		"cacheVariables": {
			"VCPKG_INSTALLED_DIR": f"vcpkg/{triplet}",
			"CMAKE_POLICY_DEFAULT_CMP0077": "NEW",
			"X_VCPKG_APPLOCAL_DEPS_INSTALL": "ON",
			"CMAKE_INSTALL_PREFIX": str(home / ".local")
		},
		"toolchainFile": str(vcpkg / "scripts/buildsystems/vcpkg.cmake"),
		"binaryDir": f"build/{triplet}"
	}
)
addPreset("buildPresets", triplet, "Debug")
addPreset("buildPresets", triplet, "Release")
addPreset("testPresets", triplet, "Debug")
addPreset("testPresets", triplet, "Release")

# save cmake presets
file = open("CMakeUserPresets.json", "w")
file.write(json.dumps(presets, indent=4))
file.close()

# configure
os.system(f"cmake --preset {triplet}")

# build and install to ~/.local/bin
os.system(f"cmake --build build/{triplet} --config Debug --target install")
