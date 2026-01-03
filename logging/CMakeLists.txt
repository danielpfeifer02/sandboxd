# Logger library - shared across all components
add_library(logger STATIC
    logger.cpp
)

target_include_directories(logger PUBLIC
    ${CMAKE_SOURCE_DIR}
)

# Link spdlog and propagate its include directories
# (needed because logger.hpp includes spdlog headers)
target_link_libraries(logger PUBLIC spdlog::spdlog)