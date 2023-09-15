cmake_minimum_required(VERSION 3.25)

# set project
project(footprinttool)

# configure languages
enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)

# enable address sanitizer
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")


# dependencies
find_package(nlohmann_json CONFIG)
if(VCPKG_TARGET_TRIPLET)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Clipper2 REQUIRED IMPORTED_TARGET Clipper2)
else()
    find_package(clipper2 CONFIG)
endif()

add_subdirectory(src)
