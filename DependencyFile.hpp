#pragma once

#include "CheckedFileStore.hpp"
#include "Context.hpp"
#include "UnorderedVector.hpp"

#include <compare>
#include <filesystem>
#include <fstream>
#include <string>

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

		static auto
		(	ReadEntry
		)	(	::std::stringstream
				&	i_rLine
			)
		->	::std::filesystem::path
		{
			::std::string
				vPath
			;
			while(getline(i_rLine, vPath, ' '))
			{
				if	(vPath.empty() or vPath[0] == '\\')
					continue;

				return ::std::filesystem::path{vPath};
			}
			return ::std::filesystem::path{};
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
		:	m_vPath
			{	i_rPath
			}
		{
			::std::ifstream
				vFile = i_rPath
			;
			::std::string
				vLine
			;
			// discard first line
			getline(vFile, vLine);

			while(getline(vFile, vLine))
			{
				::std::stringstream
					vColumn
				{	vLine
				};
				auto const
					vPath
				=	ReadEntry
					(	vColumn
					)
				;
				if	(not vPath.empty())
				{
					if	(IsImplementation(vPath.c_str()))
					{
						if	(not m_vImplementation.empty())
							::std::cerr << "\nFound more than one implementation in " << m_vPath << '\n';
						else
							m_vImplementation = vPath;
					}
					else
						m_vDependencies.push_back(vPath);
				}
			}
		}
	};


	using DependencyStore = CheckedFileStore<DependencyFile, &IsDependency>;
}
