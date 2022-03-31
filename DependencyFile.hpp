#pragma once

#include "CheckedFileStore.hpp"
#include "Context.hpp"
#include "UnorderedVector.hpp"

#include <compare>
#include <filesystem>

namespace
	Modularize
{
	struct
		DependencyFile
	{
		::std::filesystem::path
			m_vPath
		;
		::std::filesystem::path
			m_vImplementation
		;
		UnorderedVector<::std::filesystem::path>
			m_vDependencies
		;

		friend auto inline
		(	operator <=>
		)	(	DependencyFile const
				&	i_rLeft
			,	DependencyFile const
				&	i_rRight
			)
		{	return i_rLeft.m_vPath <=> i_rRight.m_vPath;	}

		explicit(false)
		(	DependencyFile
		)	()
		=	default;

		explicit(false)
		(	DependencyFile
		)	(	::std::filesystem::path const
				&	i_rPath
			)
		;
	};


	using DependencyStore = CheckedFileStore<DependencyFile, &IsDependency>;
}
