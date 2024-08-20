cmake_minimum_required(VERSION 3.10)

project(voko)

# Set the name of the executable
set(EXECUTABLE_NAME ${PROJECT_NAME})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

find_package(Vulkan REQUIRED)

# Set c++ Standard 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


IF(UNIX AND NOT APPLE)
	set(LINUX TRUE)
ENDIF()

IF(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSEIF(LINUX)
ELSEIF(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_MACOS_MVK -DVK_EXAMPLE_XCODE_GENERATED")
	# Todo : android?
ENDIF(WIN32)

# Configure Vulkan Environment
IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
ENDIF()

# Add Resource Dir Definitions
add_definitions(
	-DVOKO_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Shader/"
)
add_definitions(
	-DVOKO_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Asset/"
)

add_subdirectory(ThirdParty)

add_subdirectory(Source)

# compile glsl to spirv
add_subdirectory(Shader)

add_dependencies(voko CompileGLSL) 


