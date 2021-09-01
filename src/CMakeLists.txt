cmake_minimum_required(VERSION 3.16)

project(structs)

add_executable(${PROJECT_NAME}
	main.cpp
	parser.cpp
	struct-type.cpp
	universe.cpp
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>)

include_directories(${PROJECT_NAME} .)