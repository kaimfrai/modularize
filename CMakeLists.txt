cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(
	modularize
LANGUAGES
	CXX
)

add_executable(
	modularize
	main.cpp
	CombinedFileStore.cpp
	HeaderFile.cpp
	DependencyFile.cpp
	Directory.cpp
	ImplementationFile.cpp
	ModuleInterfaceConverter.cpp
)
target_compile_options(
	modularize
PRIVATE
	-Wall
	-Wextra
	-Wpedantic
	-Wconversion
	-Wdeprecated
	-Wmissing-variable-declarations
	-Wctad-maybe-unsupported
	-Werror
	-Weverything
	# necessary for bool data members
	-Wno-padded
	# using C++23
	-Wno-c++98-compat-pedantic
	# using c++23
	-Wno-c++20-compat-pedantic
)
target_link_options(
	modularize
PUBLIC
	-fuse-ld=lld
)
