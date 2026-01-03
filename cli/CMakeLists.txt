# CLI executable (sbx)
# This will contain the command-line interface

add_executable(sbx
    main.cpp
)

target_link_libraries(sbx
    sandbox
)

# Install the binary
install(TARGETS sbx DESTINATION bin)

