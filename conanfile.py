from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import CMake


class Project(ConanFile):
    name = "footprint-tool"
    description = "Tool for generating footprints for KiCad"
    url = "https://github.com/Jochen0x90h/footprint-tool"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"
    default_options = {}
    generators = "CMakeDeps", "CMakeToolchain"
    exports_sources = "conanfile.py", "CMakeLists.txt", "src/*"
    requires = [
        "nlohmann_json/3.12.0",
        "clipper2/2.0.1",
        #"stduuid/1.2.3",
        #"opencascade/7.9.1"
    ]

    def requirements(self):
        self.requires("opencascade/7.9.1", options={"shared": False})

    keep_imports = True
    def imports(self):
        # copy dependent libraries into the build folder
        self.copy("*", src="@bindirs", dst="bin")
        self.copy("*", src="@libdirs", dst="lib")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

        # run unit tests if CONAN_RUN_TESTS environment variable is set to 1
        #if os.getenv("CONAN_RUN_TESTS") == "1":
        #    cmake.test()

    def package(self):
        # install from build directory into package directory
        cmake = CMake(self)
        cmake.install()

        # also copy dependent libraries into the package
        #self.copy("*.dll", "bin", "bin")
        #self.copy("*.dylib*", "lib", "lib", symlinks = True)
        #self.copy("*.so*", "lib", "lib", symlinks = True)

    def package_info(self):
        pass
