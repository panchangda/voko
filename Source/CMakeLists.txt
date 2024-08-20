# get all .cpp file under Source
# file(GLOB SOURCES "*.cpp")
set(ALL_SOURCE_FILES
  voko.h 
  voko.cpp 
  debug.h
  debug.cpp
  main.cpp
  camera.hpp
  VulkanglTFModel.h
  VulkanglTFModel.cpp
  VulkanDevice.h
  VulkanDevice.cpp
  VulkanBuffer.h
  VulkanBuffer.cpp
  VulkanTools.h
  VulkanTools.cpp
  VulkanInitializers.hpp
)
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

# include 3rdparty dir
# include_directories(../ThirdParty/ktx/include)
# include_directories(../ThirdParty/ktx/other_include)
target_include_directories(${EXECUTABLE_NAME} 
  PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/ktx/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/ktx/other_include
)

# link to 3rdparty libs
target_link_libraries(${EXECUTABLE_NAME} 
PUBLIC 
glm::glm 
SDL3::SDL3 
GPUOpen::VulkanMemoryAllocator
tinygltf
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

