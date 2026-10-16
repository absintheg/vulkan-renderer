cmake_minimum_required(VERSION 3.20)
project(Game LANGUAGES C)

set(CMAKE_CXX_STANDARD, 20)
set(CMAKE_CXX_STANDARD_REQUIRED, YES)
set(CMAKE_CXX_EXTENSIONS, OFF)

find_package(SDL3 REQUIRED)

add_subdirectory(external)
add_subdirectory(client)
add_subdirectory(mem)

add_executable(${PROJECT_NAME} src/main.c)

target_link_libraries(${PROJECT_NAME}
	client
	mem
)
