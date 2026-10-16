cmake_minimum_required(VERSION 3.20)
project(Game LANGUAGES C)

set(CMAKE_C_STANDARD, 23)
set(CMAKE_C_STANDARD_REQUIRED, YES)
set(CMAKE_C_EXTENSIONS, OFF)

add_subdirectory(external)
add_subdirectory(client)
add_subdirectory(mem)

add_executable(${PROJECT_NAME} src/main.c)

target_link_libraries(${PROJECT_NAME}
	client
	mem
)
