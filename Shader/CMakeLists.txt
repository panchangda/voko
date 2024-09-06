# 设置 glslangValidator 程序的查找路径
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

# 检查 glslangValidator 是否找到
if(NOT GLSL_VALIDATOR)
  message(FATAL_ERROR "glslangValidator not found.")
endif()

# 查找所有 GLSL 文件
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "*.frag"
    "*.vert"
    "*.comp"
	  "*.geom"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "Building shader: ${GLSL}")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  get_filename_component(FILE_DIR ${GLSL} DIRECTORY)
  set(SPIRV "${FILE_DIR}/${FILE_NAME}.spv")
  #message(STATUS "Building shader from ${GLSL} to ${FILE_DIR}/${FILE_NAME}.spv")
  #message(STATUS "Command Executed: ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL}
  )
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

add_custom_target(
    CompileGLSL ALL
    DEPENDS ${SPIRV_BINARY_FILES}
)
