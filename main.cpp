#include "CombinedFileStore.hpp"
#include "FindUnused.hpp"
#include "ModuleInterfaceConverter.hpp"

#include <iostream>
#include <span>
#include <string_view>

static auto constexpr HelpText = R"(
--help          :   Display this message.

1st Argument    :   Path to project source directory.
2nd Argument    :   Path to CMake binary directory relative to the source directory. Must contain depencency files *.o.d. Use -d keepdepfile with ninja to keep those after a successful build.

--find-unsued   :   Finds C++ files that are unsued in the project.
                    Optional 3rd Argument: File listing explicitly used source files based on which the analysis is performed. Uses SourceDir/ExplicitUse.fuf by default.
--analyze       :   Groups files into adequate modules based on circular dependencies.
					Mandatory 3rd Argument: The header file based on which circular dependencies are searched.
--modularize    :   Converts all files in the source directory using header includes to C++20 modules on a best effort basis.
                    Mandatory 3rd Argument: The name of the resulting module.
                    Note that forward declarations are not checked and need to be resolved manually.
)";

auto
(	main
)	(	int argc
	,	char const
		*	argv
			[]
	)
->	int
{
	if (argc < 2)
	{
		::std::cerr << "Expected at least 1 argument. Known arguments:\n" << HelpText << ::std::endl;
		::std::exit(EXIT_FAILURE);
	}

	::std::string_view const
		vOption
	=	argv[1]
	;

	if ((vOption == "--help") or (vOption == "-h"))
	{
		std::cout << HelpText << std::endl;
		std::exit(EXIT_SUCCESS);
	}

	::std::span<char const*> const
		vArguments
	{	argv + 2uz
	,	static_cast<::std::size_t>(argc) - 2uz
	};

	if	(vOption == "--analyze")
	{
		Modularize::AnalyzeModularity
		(	vArguments
		);
	}
	else
	if	(vOption == "--convert")
	{
		Modularize::Modularize
		(	vArguments
		);
	}
	else
	if	(vOption == "--find-unused")
	{
		Modularize::FindUnused
		(	vArguments
		);
	}
	else
	{
		::std::cerr << "Unknown option: " << vOption << "\nKnown options:\n" << HelpText << ::std::endl;
		::std::exit(EXIT_FAILURE);
	}
}
