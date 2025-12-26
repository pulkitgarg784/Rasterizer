cmake_minimum_required(VERSION 3.16)
project(Rasterizer)

set(CMAKE_CXX_STANDARD 20)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Add optimization flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

# Include headers
include_directories(include)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Find OpenMP
find_package(OpenMP)

# Source files
set(SOURCES
    src/main.cpp
    src/tgaimage.cpp
    src/model.cpp
)

add_executable(Rasterizer ${SOURCES})

# Link SDL2
target_link_libraries(Rasterizer ${SDL2_LIBRARIES})

# Link OpenMP if found
if(OpenMP_CXX_FOUND)
    target_link_libraries(Rasterizer PUBLIC OpenMP::OpenMP_CXX)
endif()