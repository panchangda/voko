# get all .cpp file under Source

# recursively add all files
file(GLOB_RECURSE ALL_SOURCE_FILES "*.cpp" "*.hpp" "*.h")


# 保留相对目录结构
foreach(file ${ALL_SOURCE_FILES})
  # 获取文件的相对路径（相对于 Source 目录）
  file(RELATIVE_PATH relative_file ${CMAKE_CURRENT_SOURCE_DIR} ${file})
  
  # 打印相对路径，调试用
  #message(STATUS "Relative file: ${relative_file}")
  
  # 获取文件的目录（用于分组）
  get_filename_component(file_path ${relative_file} PATH)
  
  # 打印文件目录路径，调试用
  #message(STATUS "File path: ${file_path}")
  
  # 创建 source_group，这样 Visual Studio 中将显示文件的相对路径
  source_group(${file_path} FILES ${file})
endforeach()


add_executable(${EXECUTABLE_NAME} ${ALL_SOURCE_FILES})

set_property(TARGET ${EXECUTABLE_NAME} PROPERTY CXX_STANDARD 20)

# glm compile definitions
# force depth within [0, 1]
target_compile_definitions(${EXECUTABLE_NAME} PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_compile_definitions(${EXECUTABLE_NAME} PUBLIC GLM_FORCE_RADIANS)



# include current dir
target_include_directories(${EXECUTABLE_NAME} 
  PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

# process ktx specially, because directly add as a submodule will include too many irrelevant build targets
# include 3rdparty dirs: ktx
include_directories(${EXECUTABLE_NAME} 
  PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/ktx/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/ktx/other_include
  )

# add all ktx files as a library
  set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/ktx)
  set(KTX_SOURCES
      ${KTX_DIR}/lib/texture.c
      ${KTX_DIR}/lib/hashlist.c
      ${KTX_DIR}/lib/checkheader.c
      ${KTX_DIR}/lib/swap.c
      ${KTX_DIR}/lib/memstream.c
      ${KTX_DIR}/lib/filestream.c)
add_library(ktx STATIC ${KTX_SOURCES})

# link to 3rdparty libs
target_link_libraries(${EXECUTABLE_NAME} 
PUBLIC 
glm::glm 
SDL3::SDL3 
GPUOpen::VulkanMemoryAllocator
tinygltf
ktx  # Link the ktx library here
)


# link to vk: 
# Note that ${Vulkan_LIBRARIES} is SDL3 Bundled, Vulkan::Vulkan is System 
# target_link_libraries(${EXECUTABLE_NAME} PUBLIC ${Vulkan_LIBRARIES})
target_link_libraries(${EXECUTABLE_NAME} PUBLIC Vulkan::Vulkan)

# copy dlls to target folder, to avoid vs error: missing .dll files
add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${EXECUTABLE_NAME}> $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>
  COMMAND_EXPAND_LISTS
  )

