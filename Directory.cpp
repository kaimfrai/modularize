#include "Directory.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

namespace
{
	struct
		InvalidDirectoryException
	:	::std::runtime_error
	{
		explicit(true) inline
		(	InvalidDirectoryException
		)	()
		:	::std::runtime_error
			{	"The given path was not a valid directory!"
			}
		{}
	};
}

(	::Modularize::Directory::Directory
)	(	::std::filesystem::path const
		&	i_rPath
	)
:	m_vPath
	{	i_rPath
	}
{
	if	(	not exists(m_vPath)
		or	not is_directory(m_vPath)
		)
	{
		throw InvalidDirectoryException{};
	}
}

auto
(	::Modularize::EnsureDirectory
)	(	::std::filesystem::path const
		&	i_rPath
	,	::std::string_view
			i_vErrorMessage
	)
->	Directory
try
{
	return
	Directory
	{	i_rPath
	};
}
catch
	(	InvalidDirectoryException const
		&
	)
{
	::std::cerr << i_vErrorMessage << ::std::endl;
	::std::exit(EXIT_FAILURE);
}
