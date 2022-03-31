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
		auto
		(	HasDependency
		)	()	const
		->	bool
		{	return
			not
			m_vDependency.m_vPath.empty
			();
		}

		[[nodiscard]]
		auto
		(	GetDependencies
		)	()	const&
		->	UnorderedVector<::std::filesystem::path> const&
		{	return m_vDependency.m_vDependencies;	}

		explicit(false)
		(	ImplementationFile
		)	()
		=	default;

		explicit(false)
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
		{
			auto const
				vPosition
			=	i_rDependencyFiles.find_if
				(	[&]	(	DependencyFile const
							&	i_rDepFile
						)
					->	bool
					{
						return
							i_rDepFile.m_vImplementation.native()
						.	ends_with(m_vPath.c_str())
						;
					}
				)
			;

			if	(vPosition == i_rDependencyFiles.end())
				return false;

			m_vDependency = i_rDependencyFiles.SwapOut(vPosition);
			return true;
		}

		[[nodiscard]]
		friend auto
		(	operator<=>
		)	(	ImplementationFile const
				&	i_rLeft
			,	ImplementationFile const
				&	i_rRight
			)
		{
			return i_rLeft.m_vPath <=> i_rRight.m_vPath;
		}

		friend auto
		(	operator <<
		)	(	Modularize::DirectoryRelativeStream<decltype(::std::cout)>
				&	i_rStream
			,	ImplementationFile const
				&	i_rHeader
			)
		->	decltype(i_rStream)
		{
			return i_rStream << i_rHeader.m_vPath << '\n';
		}
	};

	using ImplementationStore = Modularize::CheckedFileStore<ImplementationFile, &IsImplementation>;
}
