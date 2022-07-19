#pragma once

#include "Directory.hpp"
#include "UnorderedVector.hpp"

#include <filesystem>
#include <string_view>

namespace
	Modularize
{
	using CheckFileFunc = auto(*)(::std::filesystem::path const&) -> bool;
	template
		<	typename
				t_tFile
		,	CheckFileFunc
				t_fCheckFile
		>
	struct
		CheckedFileStore
	;

	template
		<	typename
			...	t_tpFile
		,	CheckFileFunc
			...	t_fpCheckFile
		>
	auto
	(	PopulateFiles
	)	(	Modularize::Directory const
			&	i_rSourceDir
		,	CheckedFileStore<t_tpFile, t_fpCheckFile>
			&
			...	i_rStore
		)
	->	void
	{
		for	(	auto const
			&	i_rEntry
			:	i_rSourceDir
			)
		{
			if	(not i_rEntry.is_regular_file())
			{
				//::std::cerr << "Irregular file: " << i_rEntry.path() << std::endl;
				continue;
			}

			auto const
			&	rPath
			=	i_rEntry.path()
			;

			if	(	not
					((	...
					or	i_rStore.AddEntry(rPath)
					))
				)
			{
				//::std::cerr << "File not added: " << i_rEntry.path() << ::std::endl;
			}
		}
	}

	template
		<	typename
				t_tPath
		,	CheckFileFunc
				t_fCheckFile
		>
	struct
		CheckedFileStore
	:	Modularize::UnorderedVector<t_tPath>
	{
		explicit(false)
		(	CheckedFileStore
		)	()
		=	default;

		explicit(true)
		(	CheckedFileStore
		)	(	Directory const
				&	i_rSourceDir
			)
		{
			PopulateFiles(i_rSourceDir, *this);
		}

		auto
		(	AddEntry
		)	(	::std::filesystem::path const
				&	i_rEntry
			)	&
		->	bool
		{
			bool const bAdd = t_fCheckFile(i_rEntry);
			if	(bAdd)
				this->push_back(i_rEntry);
			return bAdd;
		}
	};
}
