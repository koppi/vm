cmake_minimum_required(VERSION 3.16.0)

project(vm VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required Qt components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# Emscripten-specific configurations
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    
    # Emscripten linker flags
    set(CMAKE_EXE_LINKER_FLAGS 
        "${CMAKE_EXE_LINKER_FLAGS} -s MAX_WEBGL_VERSION=2 -s FETCH=1 -s WASM_BIGINT=1 -s STACK_SIZE=5MB -s ALLOW_MEMORY_GROWTH"
    )
    
    # Add Qt-specific Emscripten flags
    set(CMAKE_EXE_LINKER_FLAGS 
        "${CMAKE_EXE_LINKER_FLAGS} -sASYNCIFY_IMPORTS=qt_asyncify_suspend_js,qt_asyncify_resume_js -s ERROR_ON_UNDEFINED_SYMBOLS=1"
    )
    
    # Export main function for Emscripten
    set(CMAKE_EXE_LINKER_FLAGS 
        "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','specialHTMLTargets'] -s EXPORTED_FUNCTIONS=['_main']"
    )
endif()

# Enable Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Source files
set(SOURCES
    main.cpp
    mainwindow.cpp
    vm.cpp
)

# Header files
set(HEADERS
    mainwindow.h
    vm.h
)

# UI files
set(UI_FILES
    mainwindow.ui
)

# Create executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)

# Emscripten-specific linking
if(EMSCRIPTEN)
    target_link_options(${PROJECT_NAME} PRIVATE 
        "SHELL:-lembind"
    )
endif()

# Set include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# Debug build with -g
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -g)
endif()
