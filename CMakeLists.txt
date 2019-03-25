cmake_minimum_required(VERSION 3.4)
project("Tracer")

# Compile all "*.cpp" files in the root directory:
file(GLOB_RECURSE SOURCES
    "src/*.cpp" "src/*.h"
)

set(CMAKE_CXX_STANDARD 17)
add_executable(${PROJECT_NAME} ${SOURCES})
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/Boxer")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
if (NOT WIN32)
    find_package(OpenCL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glfw3 3.2 REQUIRED)
    find_package(SDL2 REQUIRED)
    find_package(FreeImage REQUIRED)
    find_package(Threads)
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE Boxer)
include_directories(
    "${PROJECT_SOURCE_DIR}/lib/glm-0.9.9.3/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui"
)

if (WIN32)
    include_directories(
        "${PROJECT_SOURCE_DIR}/lib/FreeImage/include"
        "${PROJECT_SOURCE_DIR}/lib/OpenCL/include"
        "${PROJECT_SOURCE_DIR}/lib/OpenGL/include"
        "${PROJECT_SOURCE_DIR}/lib/SDL2-2.0.9/include"
    )
    if (CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bit
        target_link_libraries(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/lib/FreeImage/lib64/freeimage.lib")
        target_link_libraries(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/lib/OpenCL/lib/x64/OpenCL.lib")
        target_link_libraries(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/lib/OpenCL/lib/x64/glew32.lib")
        target_link_libraries(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/lib/SDL2-2.0.9/lib/x64/SDL2.lib")
        target_link_libraries(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/lib/SDL2-2.0.9/lib/x64/SDL2main.lib")
    else ()
        target_link_libraries(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/lib/FreeImage/lib32/freeimage.lib")
        target_link_libraries(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/lib/OpenCL/lib/Win32/OpenCL.lib")
        target_link_libraries(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/lib/OpenCL/lib/Win32/glew32.lib")
        target_link_libraries(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/lib/SDL2-2.0.9/lib/x86/SDL2.lib")
        target_link_libraries(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/lib/SDL2-2.0.9/lib/x86/SDL2main.lib")
    endif ()
else ()
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenCL::OpenCL)
    target_link_libraries(${PROJECT_NAME} PRIVATE GLEW::GLEW)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main)
    target_link_libraries(${PROJECT_NAME} PRIVATE FreeImage::freeimage)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ASSIMP_LIBRARIES})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT})
endif ()

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
