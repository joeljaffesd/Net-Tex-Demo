cmake_minimum_required(VERSION 3.29)

project(NDIVideoReceiverApp)
set(APP_NAME NDIVideoReceiverApp)

# Configure NDI SDK path
set(NDI_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/ndi/NDI SDK for Apple" CACHE PATH "Path to NDI SDK")

# cmake scripts are kept in the cmake directory
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# set compiler/platform specific flags
if(MSVC)
  set(CMAKE_BUILD_PARALLEL_LEVEL $ENV{NUMBER_OF_PROCESSORS})
  # add_compile_options(/utf-8)
  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:4194304")
endif()

# Add NDI Video Receiver App as the main executable
add_executable(${APP_NAME} videoPipe/NDIVideoReceiverApp.cpp)

# Set C++ standard and output properties for the main executable
set_target_properties(${APP_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/debug
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin
)

# add allolib to project & link
add_subdirectory(allolib)
target_link_libraries(${APP_NAME} PRIVATE al)

# add ndi_wrapping to project & link
add_subdirectory(videoPipe/ndi_wrapping)
target_link_libraries(${APP_NAME} PRIVATE al_ndi)

# Add include directories for the main executable
target_include_directories(${APP_NAME} PRIVATE
    allolib/include
    allolib/external/glad/include
    allolib/external/Gamma
    videoPipe/ndi_wrapping/include
    # ${CMAKE_CURRENT_LIST_DIR}/ndi/NDI SDK for Apple/include
)

# Suppress warnings for now
target_compile_options(${APP_NAME} PRIVATE -w)