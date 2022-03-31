#pragma once

#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace
	Modularize
{
	struct
		NotADirectoryException
	:	::std::runtime_error
	{
		explicit(true)
		(	NotADirectoryException
		)	()
		:	::std::runtime_error
			{	"The given path was not a directory!"
			}
		{}
	};

	class
		Directory
	{
		::std::filesystem::path
			m_vPath
		;

	public:
		explicit(true)
		(	Directory
		)	(	::std::filesystem::path const
				&	i_rPath
			)
		:	m_vPath
			{	i_rPath
			}
		{
			if	(	not exists(m_vPath)
				or	not is_directory(m_vPath))
			{
				throw NotADirectoryException{};
			}
		}

		[[nodiscard]]
		auto
		(	RelativePath
		)	(	::std::filesystem::path const
				&	i_rAbsolutePath
			)	const
		->	::std::filesystem::path
		{	return
			i_rAbsolutePath.lexically_relative
			(	m_vPath
			);
		}

		[[nodiscard]]
		friend auto
		(	operator /
		)	(	Directory const
				&	i_rDirectory
			,	::std::filesystem::path const
				&	i_rPath
			)
		->	::std::filesystem::path
		{	return
				i_rDirectory.m_vPath
			/	i_rPath
			;
		}

		[[nodiscard]]
		auto
		(	contains
		)	(	::std::filesystem::path const
				&	i_rPath
			)
			const
		->	bool
		{	return
			i_rPath.native().starts_with
			(	m_vPath.c_str()
			);
		}

		auto
		(	begin
		)	()	const
			noexcept
		->	::std::filesystem::recursive_directory_iterator
		{	return
			::std::filesystem::recursive_directory_iterator
			{	m_vPath
			};
		}

		auto
		(	end
		)	()	const
			noexcept
		->	::std::filesystem::recursive_directory_iterator
		{	return {};	}
	};

	[[nodiscard]]
	auto inline
	(	EnsureDirectory
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
		(	NotADirectoryException const
			&
		)
	{
		::std::cerr << i_vErrorMessage << ::std::endl;
		::std::exit(EXIT_FAILURE);
	}

	[[nodiscard]]
	auto inline
	(	EnsureDirectory
	)	(	::std::string_view
				i_vPath
		,	::std::string_view
				i_vErrorMessage
		)
	->	Directory
	{	return
		EnsureDirectory
		(	::std::filesystem::path
			{	i_vPath
			}
		,	i_vErrorMessage
		);
	}
}
