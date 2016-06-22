# Set project name
project (FlexEngine)
set (TARGET_NAME FlexEngine)

# Set minimum version
cmake_minimum_required (VERSION 2.8.6)
if (COMMAND cmake_policy)
    cmake_policy (SET CMP0003 NEW)
    if (CMAKE_VERSION VERSION_GREATER 2.8.12 OR CMAKE_VERSION VERSION_EQUAL 2.8.12)
        # INTERFACE_LINK_LIBRARIES defines the link interface
        cmake_policy (SET CMP0022 NEW)
    endif ()
    if (CMAKE_VERSION VERSION_GREATER 3.0.0 OR CMAKE_VERSION VERSION_EQUAL 3.0.0)
        # Disallow use of the LOCATION target property - therefore we set to OLD as we still need it
        cmake_policy (SET CMP0026 OLD)
        # MACOSX_RPATH is enabled by default
        cmake_policy (SET CMP0042 NEW)
    endif ()
endif ()

# Set CMake modules search path
set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/Modules)
set (URHO3D_HOME ${CMAKE_SOURCE_DIR}/../Urho3D_vc15_dx11)

# Include Urho3D Cmake common module
include (Urho3D-CMake-common)

# Find Urho3D library
find_package (Urho3D REQUIRED)
include_directories (${URHO3D_INCLUDE_DIRS})

# Define source files
set (SOURCE_FOLDER ${CMAKE_SOURCE_DIR}/Source/)
include_directories (${SOURCE_FOLDER})
include (SourceLists.cmake)
check_source_files ()

# Setup folders
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

source_group ("Natvis" FILES "${CMAKE_SOURCE_DIR}/Urho3D.natvis")

# Setup target with resource copying
setup_main_executable ()