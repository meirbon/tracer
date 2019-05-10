cmake_minimum_required(VERSION 3.4)
if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
		CACHE STRING "")
endif ()

project("Tracer")

# Compile all "*.cpp" files in the root directory:
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

set(CMAKE_CXX_STANDARD 17)
add_executable(${PROJECT_NAME} ${SOURCES})

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui"
	"${PROJECT_SOURCE_DIR}/lib/Assimp/include"
	"${PROJECT_SOURCE_DIR}/lib/glm-0.9.9.5/include"
	"${PROJECT_SOURCE_DIR}/lib/glfw-3.3/include"
	"${PROJECT_SOURCE_DIR}/lib/FreeImage/include"
	"${PROJECT_SOURCE_DIR}/lib/OpenCL/include"
	"${PROJECT_SOURCE_DIR}/lib/OpenGL/include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
if (WIN32)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bit
		target_link_libraries(${PROJECT_NAME} PRIVATE
			"${PROJECT_SOURCE_DIR}/lib/x64/assimp-vc140-mt.lib"
			"${PROJECT_SOURCE_DIR}/lib/x64/freeimage.lib"
			"${PROJECT_SOURCE_DIR}/lib/x64/glew32.lib"
			"${PROJECT_SOURCE_DIR}/lib/x64/glfw3dll.lib"
			"${PROJECT_SOURCE_DIR}/lib/x64/OpenCL.lib"
		)
	else()
		target_link_libraries(${PROJECT_NAME} PRIVATE
			"${PROJECT_SOURCE_DIR}/lib/Win32/assimp-vc140-mt.lib"
			"${PROJECT_SOURCE_DIR}/lib/Win32/freeimage.lib"
			"${PROJECT_SOURCE_DIR}/lib/Win32/glew32.lib"
			"${PROJECT_SOURCE_DIR}/lib/Win32/glfw3dll.lib"
			"${PROJECT_SOURCE_DIR}/lib/Win32/OpenCL.lib"
		)
	endif()
else()
	find_package(ASSIMP REQUIRED)
	find_package(ZLIB REQUIRED)
	find_package(OpenCL REQUIRED)
	find_package(GLEW REQUIRED)
	find_package(glfw3 REQUIRED)
	find_package(FreeImage REQUIRED)
	find_package(Threads)
	
	
	target_link_libraries(${PROJECT_NAME} PRIVATE
		glfw
		ZLIB::ZLIB
		OpenCL::OpenCL
		GLEW::GLEW
		FreeImage::freeimage
		${ASSIMP_LIBRARIES}
		${CMAKE_THREAD_LIBS_INIT}
	)
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
		file(GLOB DLLS "lib/dll/x64/*.dll")
		file(COPY ${DLLS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
	else ()
		file(GLOB DLLS "lib/dll/Win32/*.dll")
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
file(COPY "src/Shared.h" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/cl-src")

add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E create_symlink
		"${PROJECT_SOURCE_DIR}/models"
		"${CMAKE_CURRENT_BINARY_DIR}/models"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/models"
)

#add_custom_command(
#	TARGET ${PROJECT_NAME} POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E create_symlink
#		"${PROJECT_SOURCE_DIR}/cl-src"
#		"${CMAKE_CURRENT_BINARY_DIR}/cl-src"
#	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/cl-src"
#)
