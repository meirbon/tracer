cmake_minimum_required(VERSION 3.4)
# if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#     set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
#             CACHE STRING "")
# endif ()

project("Tracer")

# Compile all "*.cpp" files in the root directory:
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

set(CMAKE_CXX_STANDARD 17)
add_executable(${PROJECT_NAME} ${SOURCES} src/Primitives/TriangleList.cpp src/Primitives/TriangleList.h src/Core/WFTracer.cpp src/Core/WFTracer.h)
# add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/Boxer")
# add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(ASSIMP REQUIRED)
find_package(ZLIB REQUIRED)
find_package(GLM REQUIRED)
find_package(OpenCL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(FreeImage REQUIRED)
find_package(Threads)

include_directories(
        "${GLM_INCLUDE_DIRS}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui"
        "${PROJECT_SOURCE_DIR}/lib/FreeImage/include"
        "${PROJECT_SOURCE_DIR}/lib/OpenCL/include"
        "${PROJECT_SOURCE_DIR}/lib/OpenGL/include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenGL::GL
        glfw
        # Boxer
        ZLIB::ZLIB
        OpenCL::OpenCL
        GLEW::GLEW
        FreeImage::freeimage
        ${ASSIMP_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2") # AVX2 support (Intel Haswell and higher)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi") # Enable debug information on every configuration.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc") # Enable exceptions
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2") # AVX2 support (Intel Haswell and higher)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g") # Enable debug information on every configuration.`
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions") # Enable exceptions
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra") # find those pesky mistakes/typos.
endif ()

if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bit
        file(GLOB DLLS "lib/x64/*.dll")
        file(COPY ${DLLS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    else ()
        file(GLOB DLLS "lib/Win32/*.dll")
        file(COPY ${DLLS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif ()
endif ()

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

file(GLOB CL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/programs/*.cl")
file(COPY ${CL_SOURCES} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/programs")

file(GLOB GL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*")
file(COPY ${GL_SOURCES} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/shaders")
