#pragma once

#include "Directory.hpp"

#include <filesystem>
#include <string>

namespace
	Modularize
{
	auto inline
	(	IsHeader
	)	(	::std::filesystem::path const
			&	i_rPath
		)
	->	bool
	{
		::std::string const
			vFileName
		=	i_rPath.filename
			()
		;

		return
		(	vFileName.ends_with(".h")
		or	vFileName.ends_with(".hpp")
		or	not vFileName.contains(".")
		);
	}

	auto inline
	(	IsImplementation
	)	(	::std::filesystem::path const
			&	i_rPath
		)
	->	bool
	{
		::std::string const
			vFileName
		=	i_rPath.filename
			()
		;

		return
		(	vFileName.ends_with(".c")
		or	vFileName.ends_with(".cpp")
		);
	}

	auto inline
	(	IsDependency
	)	(	::std::filesystem::path const
			&	i_rPath
		)
	->	bool
	{
		::std::string const
			vFileName
		=	i_rPath.filename
			()
		;
		return
		vFileName.ends_with
		(	".o.d"
		);
	}

	class
		Context
	{
		Directory
			m_vSourceDirectory;
		;
		Directory
			m_vBinaryDirectory
		;
	public:
		explicit(true) inline
		(	Context
		)	(	Directory const
				&	i_rSourceDirectory
			,	Directory const
				&	i_rBinaryDirectory
			)
		:	m_vSourceDirectory
			{	i_rSourceDirectory
			}
		,	m_vBinaryDirectory
			{	i_rBinaryDirectory
			}
		{}

		auto inline
		(	GetSourceDirectory
		)	()	const&
		->	Directory const&
		{	return m_vSourceDirectory;	}

		auto inline
		(	GetBinaryDirectory
		)	()	const&
		->	Directory const&
		{	return m_vBinaryDirectory;	}

		auto inline
		(	StandardLog
		)	()	const
		->	DirectoryRelativeStream<decltype(::std::cerr)>
		{	return
			DirectoryRelativeStream
			{	m_vSourceDirectory
			,	::std::cerr
			};
		}

		auto inline
		(	ErrorLog
		)	()	const
		->	DirectoryRelativeStream<decltype(::std::cerr)>
		{	return
			DirectoryRelativeStream
			{	m_vSourceDirectory
			,	::std::cerr
			};
		}
	};
}
