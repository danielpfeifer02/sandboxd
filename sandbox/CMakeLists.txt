# Sandbox core library
# This will contain the core sandbox creation and management code

add_library(sandbox STATIC
    sandbox.cpp
)

target_include_directories(sandbox PUBLIC
    ${CMAKE_SOURCE_DIR}
)

