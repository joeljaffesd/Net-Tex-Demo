cmake_minimum_required(VERSION 3.29)

project(Allolib-Kickstart)
set(APP_NAME Allolib-Kickstart)

# Configure NDI SDK path for al_ext/ndi
set(NDI_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/ndi/NDI SDK for Apple" CACHE PATH "Path to NDI SDK")

# Temporarily disable problematic extensions to focus on NDI
set(AL_EXT_BUILD_OPENVR OFF CACHE BOOL "Disable OpenVR extension")
set(AL_EXT_BUILD_SOUNDFILE OFF CACHE BOOL "Disable soundfile extension") 
set(AL_EXT_BUILD_STATEDISTRIBUTION OFF CACHE BOOL "Disable statedistribution extension")
set(AL_EXT_BUILD_VIDEO OFF CACHE BOOL "Disable video extension")
set(AL_EXT_BUILD_VIDEO_VLC OFF CACHE BOOL "Disable video_vlc extension")

# cmake scripts are kept in the cmake directory
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# set compiler/platform specific flags
if(MSVC)
  set(CMAKE_BUILD_PARALLEL_LEVEL $ENV{NUMBER_OF_PROCESSORS})
  # add_compile_options(/utf-8)
  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:4194304")
endif()

# add source files to executable
# add_executable(${APP_NAME} src/main.cpp)
add_executable(${APP_NAME} al_ext/ndi/examples/NDISenderTest.cpp)

# add allolib to project & link
add_subdirectory(allolib)
target_link_libraries(${APP_NAME} PRIVATE al)

# add al_ext to project & link
if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/al_ext)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/al_ext)
  get_target_property(AL_EXT_LIBRARIES al_ext AL_EXT_LIBRARIES)
  target_link_libraries(${APP_NAME} PRIVATE ${AL_EXT_LIBRARIES})
endif()

# example line for find_package usage
# find_package(Qt5Core REQUIRED CONFIG PATHS "C:/Qt/5.12.0/msvc2017_64/lib" NO_DEFAULT_PATH)

# example of linking other libraries
# target_include_directories(${APP_NAME} PRIVATE ${PATH_TO_INCLUDE_DIR})
# target_link_libraries(${APP_NAME} PRIVATE ${PATH_TO_LIB_FILE})

# Suppress warnings for now
target_compile_options(${PROJECT_NAME} PRIVATE -w)

# binaries are put into the ./bin directory by default
set_target_properties(${APP_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/debug
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin
  # ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib
  # LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin
)