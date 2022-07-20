#include "DependencyFile.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

static auto inline
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

(	::Modularize::DependencyFile::DependencyFile
)	(	::std::filesystem::path const
		&	i_rPath
	)
:	m_vPath
	{	i_rPath
	}
{
	::std::ifstream
		vFile
	{	i_rPath
	};
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
