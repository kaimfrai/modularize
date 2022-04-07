#pragma once

#include "CheckedFileStore.hpp"
#include "DependencyFile.hpp"

#include <filesystem>

namespace
	Modularize
{
	struct
		ImplementationFile
	{
		::std::filesystem::path
			m_vPath
		;
		DependencyFile
			m_vDependency
		;

		[[nodiscard]]
		auto inline
		(	HasDependency
		)	()	const
		->	bool
		{	return
			not
			m_vDependency.m_vPath.empty
			();
		}

		[[nodiscard]]
		auto inline
		(	GetDependencies
		)	()	const&
		->	UnorderedVector<::std::filesystem::path> const&
		{	return m_vDependency.m_vDependencies;	}

		explicit(false)
		(	ImplementationFile
		)	()
		=	default;

		explicit(false) inline
		(	ImplementationFile
		)	(	::std::filesystem::path const
				&	i_rPath
			)
		:	m_vPath
			{	i_rPath
			}
		{}

		auto
		(	SetDependency
		)	(	UnorderedVector<DependencyFile>
				&	i_rDependencyFiles
			)
		->	bool
		;

		[[nodiscard]]
		friend auto inline
		(	operator<=>
		)	(	ImplementationFile const
				&	i_rLeft
			,	ImplementationFile const
				&	i_rRight
			)
		->	::std::strong_ordering
		{
			return i_rLeft.m_vPath <=> i_rRight.m_vPath;
		}

		[[nodiscard]]
		friend auto inline
		(	operator ==
		)	(	ImplementationFile const
				&	i_rLeft
			,	ImplementationFile const
				&	i_rRight
			)
		->	bool
		{
			return i_rLeft.m_vPath == i_rRight.m_vPath;
		}

		friend auto inline
		(	operator <<
		)	(	DirectoryRelativeStream<decltype(::std::cout)>
				&	i_rStream
			,	ImplementationFile const
				&	i_rHeader
			)
		->	decltype(i_rStream)
		{
			return i_rStream << i_rHeader.m_vPath << '\n';
		}
	};

	using ImplementationStore = CheckedFileStore<ImplementationFile, &IsImplementation>;
}
