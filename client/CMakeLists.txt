add_library(client STATIC
	client.c
)

target_link_libraries(client PRIVATE
	glfw
	cglm
	Vulkan::Headers
)

target_include_directories(client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
