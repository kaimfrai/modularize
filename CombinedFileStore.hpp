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
		{
			PopulateFiles(i_rSourceDir, vHeaderFiles, vImplementationFiles);
			PopulateFiles(i_rBinaryDir, vDependencyFiles);
			for	(	auto
					&	rHeader
				:	vHeaderFiles
				)
			{
				if	(rHeader.SetImplementation(vImplementationFiles))
					rHeader.SetDependency(vDependencyFiles);
			}
			for	(	auto
					&	rImplementation
				:	vImplementationFiles
				)
			{
				rImplementation.SetDependency(vDependencyFiles);
			}
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
