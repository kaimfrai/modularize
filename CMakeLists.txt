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
	FindUnused.cpp
)
