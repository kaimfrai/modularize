#pragma once

#include "DependencyFile.hpp"
#include "Directory.hpp"
#include "HeaderFile.hpp"
#include "ImplementationFile.hpp"

#include <span>

namespace
	Modularize
{
	struct
		CombinedFileStore
	{
		HeaderStore
			vHeaderFiles
		;

		ImplementationStore
			vImplementationFiles
		;

		DependencyStore
			vDependencyFiles
		;

		explicit(true)
		(	CombinedFileStore
		)	(	Directory const
				&	i_rSourceDir
			,	Directory const
				&	i_rBinaryDir
			)
		;

		auto
		(	SwapOutImplementation
		)	(	std::filesystem::path const
				&	i_rPath
			)
		->	ImplementationFile
		;

		auto
		(	SwapOutHeader
		)	(	std::filesystem::path const
				&	i_rPath
			)
		->	HeaderFile
		{
			return vHeaderFiles.SwapOut(i_rPath);
		}
	};

	auto
	(	AnalyzeModularity
	)	(	::std::span
			<	char const*
			>	i_vArgument
		)
	->	void
	;

}
