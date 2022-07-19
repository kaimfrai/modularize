#include "FindUnused.hpp"

#include "CombinedFileStore.hpp"
#include "Directory.hpp"

#include <algorithm>
#include <ranges>
#include <iostream>
#include <string_view>
#include <filesystem>
#include <vector>
#include <queue>
#include <fstream>

static auto
(	PrintUnusedFiles
)	(	Modularize::Directory const
		&	i_rPrefix
	,	Modularize::HeaderStore
		&	i_rUnusedHeaders
	,	Modularize::ImplementationStore
		&	i_rUnusedImplementations
	)
->	void
{
	Modularize::DirectoryRelativeStream out{i_rPrefix, std::cout};

	if	(not i_rUnusedHeaders.empty())
	{
		i_rUnusedHeaders.sort();
		out << "\nUnused header files:\n";
		for (	auto const
				&	rHeader
			:	i_rUnusedHeaders
			)
			out << '\t' << rHeader << '\n';
	}

	if	(not i_rUnusedImplementations.empty())
	{
		i_rUnusedImplementations.sort();

		out << "\nUnused source files:\n";
		for (	auto const
				&	rImpl
			:	i_rUnusedImplementations
			)
			out << '\t' << rImpl << '\n';
	}
}

static auto
(	EraseFilesInUse
)	(	Modularize::CombinedFileStore
		&	i_rFileStore
	,	Modularize::Directory const
		&	i_rPrefix
	)
{
	auto vUnusedHeaders = i_rFileStore.vHeaderFiles;
	auto vUnusedImplementations = i_rFileStore.vImplementationFiles;

	vUnusedHeaders.erase_if
	(	[	&i_rFileStore
		]	(	Modularize::HeaderFile const
				&	i_rUnusedHeader
			)
		{
			auto const
				fUsesHeader
			=	[	&i_rUnusedHeader
				]	(	auto const
						&	i_rFile
					)
				{	return
					i_rFile.GetDependencies().contains
					(	i_rUnusedHeader.m_vPath
					);
				}
			;
			return
				std::ranges::any_of
				(	i_rFileStore.vHeaderFiles
				,	fUsesHeader
				)
			or	std::ranges::any_of
				(	i_rFileStore.vImplementationFiles
				,	fUsesHeader
				)
			;
		}
	);

	vUnusedImplementations.erase_if
	(	[]	(	Modularize::ImplementationFile const
				&	i_rImplementation
			)
		{
			// used if it has a dependency path
			return i_rImplementation.m_vDependency.m_vPath.empty();
		}
	);

	PrintUnusedFiles
	(	i_rPrefix
	,	vUnusedHeaders
	,	vUnusedImplementations
	);

	for	(	auto const
			&	rUnusedHeader
		:	vUnusedHeaders
		)
	{
		i_rFileStore.vHeaderFiles.SwapOut(rUnusedHeader);
	}
	for	(	auto const
			&	rUnusedImplementation
		:	vUnusedImplementations
		)
	{
		i_rFileStore.vImplementationFiles.SwapOut(rUnusedImplementation);
	}
}

static auto inline
(	PopHeaderImplementation
)	(	Modularize::CombinedFileStore
		&	i_rFileStore
	,	::std::filesystem::path const
		&	i_rHeader
	)
->	Modularize::ImplementationFile
{
	auto const
		vPosition
	=	::std::ranges::find_if
		(	i_rFileStore.vHeaderFiles
		,	[i_rHeader]
			(	Modularize::HeaderFile const
				&	i_rElement
			)
			{
				return i_rHeader == i_rElement.m_vPath;
			}
		)
	;

	return i_rFileStore.vHeaderFiles.SwapOut(vPosition).m_vImplementation;
}

static auto
(	FindUnusedFiles
)	(	Modularize::CombinedFileStore
		&	i_rFileStore
	,	std::queue<Modularize::ImplementationFile>
		&	i_rImplementationQueue
	)
->	void
{
	for	(
		;	not i_rImplementationQueue.empty()
		;	i_rImplementationQueue.pop()
		)
	{
		auto const
			vElement
		=	i_rImplementationQueue.front()
		;

		for (	auto const
				&	rDependency
			:	vElement.GetDependencies()
			)
		{
			if	(	auto const
						vImplementation
					=	PopHeaderImplementation(i_rFileStore, rDependency)
				;	not vImplementation.m_vPath.empty()
				)
			{
				i_rImplementationQueue.push(vImplementation);
			}
		}
	}
}

auto inline
(	PrintResults
)	(	Modularize::CombinedFileStore
		&	i_rFileStore
	,	Modularize::Directory const
		&	i_rPrefix
	)
->	void
{
	PrintUnusedFiles(i_rPrefix, i_rFileStore.vHeaderFiles, i_rFileStore.vImplementationFiles);
}

static auto
(	InitializeExplicitFiles
)	(	Modularize::CombinedFileStore
		&	i_rFileStore
	,	::std::queue<Modularize::ImplementationFile>
		&	i_rImplementationQueue
	,	Modularize::Directory const
		&	i_rPrefix
	,	::std::filesystem::path const
		&	i_rExplicitUse
	)
->	void
{
	using namespace Modularize;
	::std::ifstream
		vFile = i_rExplicitUse
	;
	::std::string
		sExplicit
	;
	while(getline(vFile, sExplicit))
	{
		//	skip empty lines, # for comments
		if (sExplicit.empty() or sExplicit.starts_with("#"))
			continue;

		auto const vExplicitFile = i_rPrefix / sExplicit;

		if	(	not exists(vExplicitFile)
			or	not is_regular_file(vExplicitFile)
			)
		{
			::std::cerr << "Explicit file " << vExplicitFile << " not found!" << ::std::endl;
			::std::exit(EXIT_FAILURE);
		}
		else
		if	(IsImplementation(vExplicitFile))
		{
			i_rImplementationQueue.push
			(	i_rFileStore.SwapOutImplementation(vExplicitFile)
			);
		}
		else
		if	(IsHeader(vExplicitFile))
		{
			i_rFileStore.SwapOutHeader
			(	vExplicitFile
			);
		}
		else
		{
			::std::cerr << "Extension of explict file " << vExplicitFile << " not recognized!" << std::endl;
		}
	}
}

auto
(	::Modularize::FindUnused
)	(	::std::span<char const*>
			i_vArgument
	)
->	void
{
	if	(	i_vArgument.size()
		<	2uz
		)
	{
		::std::cerr << "Path to source directory, relative binary directory, and optionally explicit uses file required as arguments!" << ::std::endl;
		::std::exit(EXIT_FAILURE);
	}

	Directory const vSourceDir = EnsureDirectory(::std::string_view{i_vArgument[0uz]}, "Source directory required as first argument!");
	Directory const vBinaryDir = EnsureDirectory(vSourceDir / ::std::string_view{i_vArgument[1uz]}, "Relative binary directory required as second argument!");

	auto const
		vExplicitUse
	=	vSourceDir
	/	(	i_vArgument.size() > 2uz
		?	i_vArgument[2uz]
		:	"ExplicitUse.fuf"
		)
	;

	if(not exists(vExplicitUse) or not is_regular_file(vExplicitUse))
	{
		::std::cerr << "Explicit use file " << vExplicitUse << " not found!" << ::std::endl;

		::std::exit(EXIT_FAILURE);
	}

	CombinedFileStore
		vFileStore
	{	vSourceDir
	,	vBinaryDir
	};

	::std::queue<ImplementationFile>
		vImplementationQueue
	;

	InitializeExplicitFiles
	(	vFileStore
	,	vImplementationQueue
	,	vSourceDir
	,	vExplicitUse
	);

	EraseFilesInUse(vFileStore, vSourceDir);

	FindUnusedFiles
	(	vFileStore
	,	vImplementationQueue
	);
	PrintResults
	(	vFileStore
	,	vSourceDir
	);
}
