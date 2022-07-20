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
		,	[	i_rHeader
			]
			(	Modularize::HeaderFile const
				&	i_rElement
			)
			->	bool
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
		vFile
	{	i_rExplicitUse
	};
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
			auto const vHeader
			=	i_rFileStore.SwapOutHeader
				(	vExplicitFile
				)
			;
			if (not vHeader.m_vImplementation.m_vPath.empty())
				i_rImplementationQueue.push(vHeader.m_vImplementation);
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

	FindUnusedFiles
	(	vFileStore
	,	vImplementationQueue
	);
	PrintResults
	(	vFileStore
	,	vSourceDir
	);
}
