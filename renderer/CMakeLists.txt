add_library(renderer STATIC
	renderer.c
)

target_link_libraries(renderer PRIVATE
	mem
	cglm
	volk
	Vulkan::Headers
)

target_include_directories(renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
