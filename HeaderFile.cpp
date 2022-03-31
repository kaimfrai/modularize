#include "HeaderFile.hpp"

#include <filesystem>
#include <iostream>

namespace
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
}

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
