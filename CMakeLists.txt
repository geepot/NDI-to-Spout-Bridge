cmake_minimum_required(VERSION 3.10)
project(SpoutToNDIBridge)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add Windows-specific configurations
if(WIN32)
    # Add WIN32 to create a Windows GUI application
    set(GUI_TYPE WIN32)
    
    # Add Windows-specific compiler flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif()

# Define paths for external libraries
set(SPOUT_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/Spout-SDK-binaries/2-007-015")
set(SPOUT_INCLUDE_PATH "${SPOUT_SDK_PATH}/Libs/include")
set(SPOUT_LIB_PATH "${SPOUT_SDK_PATH}/Libs/MT")

# Check both Program Files locations for NDI SDK
if(EXISTS "C:/Program Files/NDI/NDI 6 SDK")
    set(NDI_SDK_PATH "C:/Program Files/NDI/NDI 6 SDK")
    message(STATUS "Found NDI SDK in Program Files")
elseif(EXISTS "C:/Program Files (x86)/NDI/NDI 6 SDK")
    set(NDI_SDK_PATH "C:/Program Files (x86)/NDI/NDI 6 SDK")
    message(STATUS "Found NDI SDK in Program Files (x86)")
else()
    message(FATAL_ERROR "Could not find NDI SDK. Please install it in Program Files or Program Files (x86)")
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# Print paths for debugging
message(STATUS "SPOUT_SDK_PATH: ${SPOUT_SDK_PATH}")
message(STATUS "SPOUT_INCLUDE_PATH: ${SPOUT_INCLUDE_PATH}")
message(STATUS "SPOUT_LIB_PATH: ${SPOUT_LIB_PATH}")
message(STATUS "NDI_SDK_PATH: ${NDI_SDK_PATH}")

# Add compile definitions
add_definitions(
    -DNOMINMAX
    -DWIN32_LEAN_AND_MEAN
)

# Include directories in correct order
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENGL_INCLUDE_DIR}
    "${SPOUT_INCLUDE_PATH}/SpoutLibrary"
    "${SPOUT_INCLUDE_PATH}/SpoutGL"
    "${SPOUT_INCLUDE_PATH}"
    "${NDI_SDK_PATH}/Include"
)

# Add library directories
link_directories(
    "${SPOUT_LIB_PATH}/lib"
    "${NDI_SDK_PATH}/Lib/x64"
)

# Add source files
set(SOURCE_FILES
    src/main.cpp
    src/BridgeInstance.cpp
    src/ListView.cpp
    src/DialogHandlers.cpp
    src/resource.rc
)

# Create executable
add_executable(${PROJECT_NAME} ${GUI_TYPE} ${SOURCE_FILES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    SpoutLibrary
    Processing.NDI.Lib.x64
    comctl32  # For common controls
    d3d11     # DirectX 11
    dxgi      # DirectX Graphics Infrastructure
)

# Add post-build commands to copy DLLs
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SPOUT_LIB_PATH}/bin/SpoutLibrary.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/SpoutLibrary.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${NDI_SDK_PATH}/Bin/x64/Processing.NDI.Lib.x64.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/Processing.NDI.Lib.x64.dll
)

# Print include directories for debugging
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "Include dir: ${dir}")
endforeach()
