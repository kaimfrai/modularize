#include "CombinedFileStore.hpp"
#include "ModuleInterfaceConverter.hpp"

#include <iostream>
#include <span>
#include <string_view>

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
		::std::cerr << "Expected at least 1 argument. Known arguments: --analyze." << ::std::endl;
		::std::exit(EXIT_FAILURE);
	}

	::std::string_view const
		vOption
	=	argv[1]
	;

	::std::span<char const*> const
		vArguments
	{	argv + 2uz
	,	argc - 2uz
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
	{
		::std::cerr << "Unknown option: " << vOption << ::std::endl;
		::std::exit(EXIT_FAILURE);
	}
}
