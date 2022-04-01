#include "ImplementationFile.hpp"

#include "DependencyFile.hpp"
#include "UnorderedVector.hpp"

auto
(	::Modularize::ImplementationFile::SetDependency
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
