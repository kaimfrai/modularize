#pragma once

#include "CheckedFileStore.hpp"
#include "ImplementationFile.hpp"

#include <filesystem>
#include <string_view>

namespace
	Modularize
{

	static auto
	(	SearchImplementation
	)	(	::std::filesystem::path
				i_vBase
		,	Modularize::UnorderedVector<ImplementationFile>
			&	i_rImplementationFiles
		)
	->	typename Modularize::UnorderedVector<ImplementationFile>::iterator
	{
		i_vBase.replace_extension();
		::std::string
			vSameDir
		=	i_vBase
		;

		auto const
			fPriority1
		=	[	&vSameDir
			]	(	ImplementationFile const
					&	i_rPath
				)
			->	bool
			{
				::std::filesystem::path vNoExt = i_rPath.m_vPath;
				vNoExt.replace_extension();
				return vSameDir == vNoExt;
			}
		;

		if	(	auto const
					vIncludePos
				=	vSameDir.find("include")
			;		vIncludePos
				<	vSameDir.length()
			)
		{
			::std::string
				vSrcDir
			=	vSameDir
			;
			vSrcDir.replace(vIncludePos, 7uz, "src");

			auto const
				fPriority2
			=	[	vSrcDir
				]	(	ImplementationFile const
						&	i_rPath
					)
				->	bool
				{
					::std::filesystem::path vNoExt = i_rPath.m_vPath;
					vNoExt.replace_extension();
					return vSrcDir == vNoExt;
				}
			;

			vSrcDir.replace(vIncludePos, 4uz, "");
			auto const
				fPriority3
			=	[	vSrcDir
				]	(	ImplementationFile const
						&	i_rPath
					)
				->	bool
				{
					::std::filesystem::path vNoExt = i_rPath.m_vPath;
					vNoExt.replace_extension();
					return vSrcDir == vNoExt;
				}
			;

			return
			i_rImplementationFiles.FindByPriority
			(	fPriority1
			,	fPriority2
			,	fPriority3
			);
		}
		else
			return
			i_rImplementationFiles.FindByPriority
			(	fPriority1
			);
	}

	struct
		HeaderFile
	{
		::std::filesystem::path
			m_vPath
		;
		ImplementationFile
			m_vImplementation
		;

		[[nodiscard]]
		auto inline
		(	IsHeaderOnly
		)	()	const
		->	bool
		{	return
			m_vImplementation.m_vPath.empty
			();
		}

		[[nodiscard]]
		auto inline
		(	HasDependency
		)	()	const
		->	bool
		{	return
			m_vImplementation.HasDependency
			();
		}

		[[nodiscard]]
		auto inline
		(	GetDependencies
		)	()	const&
		->	UnorderedVector<::std::filesystem::path> const&
		{	return
			m_vImplementation.GetDependencies
			();
		}

		[[nodiscard]]
		friend auto inline
		(	operator<=>
		)	(	HeaderFile const
				&	i_rLeft
			,	HeaderFile const
				&	i_rRight
			)
		->	::std::strong_ordering
		{	return
				i_rLeft.m_vPath
			<=>	i_rRight.m_vPath
			;
		}

		[[nodiscard]]
		friend auto inline
		(	operator ==
		)	(	HeaderFile const
				&	i_rLeft
			,	HeaderFile const
				&	i_rRight
			)
		->	bool
		{	return
				i_rLeft.m_vPath
			==	i_rRight.m_vPath
			;
		}

		explicit(false)
		(	HeaderFile
		)	()
		=	default;

		explicit(false)
		(	HeaderFile
		)	(	::std::filesystem::path const
				&	i_rPath
			)
		;

		auto
		(	SetImplementation
		)	(	UnorderedVector<ImplementationFile>
				& i_rImplementationFiles
			)
		->	bool
		;

		auto inline
		(	SetDependency
		)	(	UnorderedVector<DependencyFile>
				&	i_rDependencyFiles
			)
		->	bool
		{	return
			m_vImplementation.SetDependency
			(	i_rDependencyFiles
			);
		}

		friend auto inline
		(	operator <<
		)	(	Modularize::DirectoryRelativeStream<decltype(::std::cout)>
				&	i_rStream
			,	HeaderFile const
				&	i_rHeader
			)
		->	decltype(i_rStream)
		{

			i_rStream << i_rHeader.m_vPath << '\n';
			if	(	not
					i_rHeader.IsHeaderOnly()
				)
				i_rStream << i_rHeader.m_vImplementation;
			return i_rStream;
		}
	};

	auto
	(	EnsureHeader
	)	(	::std::filesystem::path const
			&	i_rPath
		,	::std::string_view
				i_sErrorMessage
		)
	->	HeaderFile
	;

	using HeaderStore = CheckedFileStore<HeaderFile, &IsHeader>;
}
