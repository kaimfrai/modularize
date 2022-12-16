# Modularize

## Überblick
This project provides the program modulularize. Its primary goal is to help modernize a C++ project using C++20 Modules. There are 3 options to choose from:

```--find-unsued```

Finds files in the project which are no longer needed. It's possible to specify a list of explicitly required files. By default, a file ExplicitUse.fuf will be loaded.

```--analyse```
Groups together files with cyclic dependencies into modules. Starts from one file and categorizes other files based on their dependencies to the first file.

```--modularize```
Converts all C++ files in the project based on their #include directives to using C++20 modules. Converted files may require manual adjustments as this is a prototype and not perfect. Hower, it will lower the work load of transitioning significantly.

## Dependencies

- cmake >=3.20
- clang >=15
- ninja-build

## Build

To build the project sue the following commands in the project directory:

```
> mkdir build
> cmake -S ./ -B ./build  --toolchain=CMake/toolchain/clang.cmake -G="Ninja"
> cd build
> ninja -d keepdepfile
``` 

## Requirements
Dependency files must exist in the project. Ninja may generate these with the following command:

```
> ninja -d keepdepfile
```

## Application
Go to the build directory and execute the following commands after adjusting the paths to your project.

```
./modularize --find-unused "path/to/source" "relative/path/to/binaries" "relative/path/to/explicit/file (optional)"
./modularize --analyze "path/to/source" "relative/path/to/binaries" "relative/path/to/some/header"
./modularize --convert "path/to/source" "relative/path/to/binaries" "Desired.Root.Module.Name"
```
