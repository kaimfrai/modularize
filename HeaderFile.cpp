#include "HeaderFile.hpp"

#include <filesystem>
#include <iostream>

namespace
	Modularize
{	namespace
{
	struct
		InvalidHeaderFileException
	:	::std::runtime_error
	{
		explicit(true) inline
		(	InvalidHeaderFileException
		)	()
		:	::std::runtime_error
			{	"The given path was not a valid header file!"
			}
		{}
	};

	auto
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
}}

(	::Modularize::HeaderFile::HeaderFile
)	(	::std::filesystem::path const
		&	i_rPath
	)
:	m_vPath{i_rPath}
{
	if	(	not exists(i_rPath)
		or	not IsHeader(i_rPath)
		)
	{
		throw InvalidHeaderFileException{};
	}
}

auto
(	::Modularize::HeaderFile::SetImplementation
)	(	UnorderedVector<ImplementationFile>
		& i_rImplementationFiles
	)
->	bool
{
	auto vImplementationIt = SearchImplementation(m_vPath, i_rImplementationFiles);
	if	(vImplementationIt == i_rImplementationFiles.end())
		return false;

	m_vImplementation = i_rImplementationFiles.SwapOut(vImplementationIt);
	return true;
}

auto
(	::Modularize::EnsureHeader
)	(	::std::filesystem::path const
		&	i_rPath
	,	::std::string_view
			i_sErrorMessage
	)
->	HeaderFile
try
{
	return{ i_rPath };
}
catch
	(	InvalidHeaderFileException const
		&
	)
{
	::std::cerr << i_sErrorMessage << ::std::endl;
	::std::exit(EXIT_FAILURE);
}
