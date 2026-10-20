add_library(loader STATIC
	loader.c
)

target_link_libraries(loader PRIVATE
	mem
)

target_include_directories(loader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
