project (FlexEngine)
cmake_minimum_required(VERSION 3.2.3)

#
# Setup CMake
#

cmake_policy (SET CMP0003 NEW)
# INTERFACE_LINK_LIBRARIES defines the link interface
cmake_policy (SET CMP0022 NEW)
# Disallow use of the LOCATION target property - therefore we set to OLD as we still need it
cmake_policy (SET CMP0026 OLD)
# MACOSX_RPATH is enabled by default
cmake_policy (SET CMP0042 NEW)
# Automatically link Qt executables to qtmain target on Windows - don't need it.
cmake_policy (SET CMP0020 OLD)

#
# Setup Urho3D
#

set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/Modules)
set (URHO3D_HOME ${CMAKE_SOURCE_DIR}/../Urho3D)
set (URHO3D_SOURCE ${URHO3D_HOME}_source)

file(COPY ${URHO3D_SOURCE}/CMake DESTINATION ${CMAKE_SOURCE_DIR})
include (Urho3D-CMake-common)

#
# Setup Player
#

set (SOURCE_FOLDER ${CMAKE_SOURCE_DIR}/Source/)
file (GLOB_RECURSE FLEX_SRC
    ${CMAKE_SOURCE_DIR}/*.natvis
    ${SOURCE_FOLDER}/*.h
    ${SOURCE_FOLDER}/*.cpp
)
include_directories (${SOURCE_FOLDER})
set (SOURCE_FILES ${FLEX_SRC})

foreach (SOURCE_FILE ${SOURCE_FILES})
    string (REPLACE "${SOURCE_FOLDER}" "" PATH ${SOURCE_FILE})
    get_filename_component (PATH ${PATH} PATH)
    if (PATH)
        string (REPLACE / \\ PATH ${PATH})
        source_group ("${PATH}" FILES ${SOURCE_FILE})
    else ()
        source_group ("Source" FILES ${SOURCE_FILE})
    endif ()
endforeach ()

source_group ("Natvis" FILES ${CMAKE_SOURCE_DIR}/Urho3D.natvis)

set (TARGET_NAME FlexEnginePlayer)
setup_main_executable ()
