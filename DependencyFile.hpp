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
		->	std::weak_ordering
		{	auto const nCmp = i_rLeft.m_vPath.native().compare(i_rRight.m_vPath.native());
			if (nCmp < 0)
				return std::weak_ordering::less;
			if (nCmp > 0)
				return std::weak_ordering::greater;
			return std::weak_ordering::equivalent;
		}

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
