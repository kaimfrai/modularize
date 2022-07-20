set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR})

#resolve real path for clang-tidy
find_program(CMAKE_C_COMPILER clang-15 REQUIRED)
file(REAL_PATH ${CMAKE_C_COMPILER} CMAKE_C_COMPILER EXPAND_TILDE)
find_program(CMAKE_CXX_COMPILER clang++-15 REQUIRED)
file(REAL_PATH ${CMAKE_CXX_COMPILER} CMAKE_CXX_COMPILER EXPAND_TILDE)

#do not use libc++ for C objects
add_compile_options(-stdlib=libc++)
add_link_options(-stdlib=libc++)
add_link_options(-lm)
add_link_options(-lc++)
add_link_options(-lc++abi)
add_link_options(-fuse-ld=lld)

add_compile_options(
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
add_compile_options(-Werror)

