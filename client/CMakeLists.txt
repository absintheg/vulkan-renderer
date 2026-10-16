add_library(client STATIC
	client.c
)

target_link_libraries(client PRIVATE
	SDL3::SDL3
)

target_include_directories(client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
