
#include "CombinedFileStore.hpp"
#include "CheckedFileStore.hpp"
#include "DependencyFile.hpp"
#include "Directory.hpp"
#include "HeaderFile.hpp"
#include "ImplementationFile.hpp"
#include "UnorderedVector.hpp"

#include <iostream>
#include <string_view>
#include <filesystem>

auto
(	main
)	(	int argc
	,	char const
		*	argv
			[]
	)
->	int
{
	if (argc < 4)
	{
		::std::cerr << "Path to source directory, binary directory, and relative header required as arguments!" << ::std::endl;
		return EXIT_FAILURE;
	}


	Modularize::Directory const SourceDir = Modularize::EnsureDirectory(::std::string_view{argv[1]}, "Source directory required as first argument!");
	Modularize::Directory const BinaryDir = Modularize::EnsureDirectory(SourceDir / ::std::string_view{argv[2]}, "Relative binary directory required as second argument!");
	Modularize::HeaderFile const vRootHeaderPath = Modularize::EnsureHeader(SourceDir / ::std::string_view{argv[3]}, "Relative header required as third argument!");

	Modularize::CombinedFileStore
		vAllFiles
	{	SourceDir
	,	BinaryDir
	};
	Modularize::DirectoryRelativeStream
		cerr
	{	SourceDir
	,	::std::cerr
	};

	Modularize::DirectoryRelativeStream
		cout
	{	SourceDir
	,	::std::cout
	};

	auto const vRootHeader = vAllFiles.vHeaderFiles.SwapOut(vRootHeaderPath);

	if	(vRootHeader.IsHeaderOnly())
	{
		cerr << "Could not find an implementation file for " << vRootHeader << ::std::endl;
		::std::exit(EXIT_FAILURE);
	}
	if	(not vRootHeader.HasDependency())
	{
		cerr << "Could not find a dependency file for " << vRootHeader.m_vImplementation << ::std::endl;
		::std::exit(EXIT_FAILURE);
	}

	cout << "Required files:\n";
	cout << vRootHeader;

	Modularize::UnorderedVector<Modularize::HeaderFile>
		vDependentOnHeaders
	;

	auto const& rRootDependencies = vRootHeader.m_vImplementation.m_vDependency.m_vDependencies;
	vDependentOnHeaders.reserve(rRootDependencies.size());
	for	(	auto const
			&	rHeaderPath
		:	rRootDependencies
		)
	{
		vDependentOnHeaders.push_back
		(	vAllFiles.vHeaderFiles.SwapOut
			(	Modularize::HeaderFile{rHeaderPath}
			)
		);
	}

	Modularize::UnorderedVector<Modularize::HeaderFile>
		vRequiredHeaders
	;
	vRequiredHeaders.push_back(vRootHeader);

	Modularize::UnorderedVector<Modularize::HeaderFile>
		vHeaderOnly
	;

	//	remove headers from other directories
	for	(	auto
				nDependentOnIndex
			=	0uz
		;		nDependentOnIndex
			<	vDependentOnHeaders.size()
		;	// may erase in loop
		)
	{
		auto const& rDependentOnHeader = vDependentOnHeaders[nDependentOnIndex];
		if	(SourceDir.contains(rDependentOnHeader.m_vPath))
		{
			++nDependentOnIndex;
		}
		else
		{
			vDependentOnHeaders.SwapOut(nDependentOnIndex);
		}
	}

	//	may grow over time
	for	(	auto
				nRequiredIndex
			=	0uz
		;		nRequiredIndex
			<	vRequiredHeaders.size()
		;	// may reset in loop
		)
	{
		auto const& rRequiredHeader = vRequiredHeaders[nRequiredIndex];
		++nRequiredIndex;
		for	(	auto
					nDependentOnIndex
				=	0uz
			;		nDependentOnIndex
				<	vDependentOnHeaders.size()
			;	// may erase in loop
			)
		{
			auto const& rDependentOnHeader = vDependentOnHeaders[nDependentOnIndex];

			if	(rDependentOnHeader.IsHeaderOnly())
			{
				auto const vDependentOnHeader = vDependentOnHeaders.SwapOut(nDependentOnIndex);
				// header only not handled
				if	(	SourceDir.contains(vDependentOnHeader.m_vPath)
					and	(	not
							vHeaderOnly.contains
							(	vDependentOnHeader
							)
						)
					)
				{
					vHeaderOnly.push_back(::std::move(vDependentOnHeader));
				}
				continue;
			}

			if (not rDependentOnHeader.HasDependency())
			{
				cerr << "No dependency file found for " << rDependentOnHeader.m_vImplementation;
				vDependentOnHeaders.SwapOut(nDependentOnIndex);
				continue;
			}

			auto const
			&	rDependentOnDependencies
			=	rDependentOnHeader.GetDependencies()
			;

			// not a cyclic dependency (yet)
			if	(	rDependentOnDependencies.none_of
					(	[	&rRequiredHeader
						]	(	::std::filesystem::path const
								&	i_rDependentOnDependency
							)
						{	return i_rDependentOnDependency == rRequiredHeader.m_vPath;	}
					)
				)
			{
				++nDependentOnIndex;
				continue;
			}


			//	add dependencies of new required header
			for (	auto const
					&	rDependentOnDependency
				:	rDependentOnDependencies
				)

			{
				if	(	//	dont add headers in other directories
						SourceDir.contains(rDependentOnDependency)
					and	not
						vDependentOnHeaders.contains
						(	rDependentOnDependency
						)
					and	not
						vRequiredHeaders.contains
						(	rDependentOnDependency
						)
					)
				{
					vDependentOnHeaders.push_back
					(	vAllFiles.vHeaderFiles.SwapOut
						(	Modularize::HeaderFile{rDependentOnDependency}
						)
					);
				}
			}

			// add to required headers
			cout << rDependentOnHeader;
			vRequiredHeaders.push_back
			(	vDependentOnHeaders.SwapOut
				(	nDependentOnIndex
				)
			);

			// restart loop accounting for new required header
			nRequiredIndex = 0;
			break;
		}
	}

	cout << "\nPure file dependencies:\n";
	for	(	auto const
			&	rLeftOver
		:	vDependentOnHeaders
		)
	{
		cout << rLeftOver;
	}

	Modularize::UnorderedVector<Modularize::HeaderFile>
		vInverseFileDependencies
	;
	cout << "\nInverse file dependencies:\n";
	for	(	auto
				nLeftOverIndex
			=	0uz
		;		nLeftOverIndex
			<	vAllFiles.vHeaderFiles.size()
		;
		)
	{
		auto const& rDependencies = vAllFiles.vHeaderFiles[nLeftOverIndex].GetDependencies();
		auto const
			fIsDependency
		=	[	&rDependencies
			]	(	Modularize::HeaderFile const
					&	i_rRequired
				)
			{
				return rDependencies.contains(i_rRequired.m_vPath);
			}
		;
		if	(	vRequiredHeaders.any_of
				(	fIsDependency
				)
			or	vInverseFileDependencies.any_of
				(	fIsDependency
				)
			)
		{
			auto vFile = vAllFiles.vHeaderFiles.SwapOut(nLeftOverIndex);
			cout << vFile;
			vInverseFileDependencies.push_back(::std::move(vFile));
			//	restart loop
			nLeftOverIndex = 0uz;
		}
		else
			++nLeftOverIndex;
	}

	cout << "\nInverse implementation file dependencies without header:\n";
	for	(	auto
				nLeftOverIndex
			=	0uz
		;		nLeftOverIndex
			<	vAllFiles.vImplementationFiles.size()
		;
		)
	{
		auto const& rDependencies = vAllFiles.vImplementationFiles[nLeftOverIndex].GetDependencies();
		if	(	vRequiredHeaders.any_of
				(	[	&rDependencies
					]	(	Modularize::HeaderFile const
							&	i_rRequired
						)
					{
						return rDependencies.contains(i_rRequired.m_vPath);
					}
				)
			)
		{
			cout << vAllFiles.vImplementationFiles.SwapOut(nLeftOverIndex);
		}
		else
			++nLeftOverIndex;
	}

	cout << "\nExclusive header only file dependencies:\n";
	for	(	auto
				nLeftOverIndex
			=	0uz
		;		nLeftOverIndex
			<	vHeaderOnly.size()
		;
		)
	{
		auto const
			fDependsOnHeader
		=	[	&vHeaderPath
			=	vHeaderOnly[nLeftOverIndex].m_vPath
			]	(	auto const
					&	i_rLeftOverFile
				)
			{
				return i_rLeftOverFile.GetDependencies().contains(vHeaderPath);
			}
		;

		if	(	vDependentOnHeaders.none_of
				(	fDependsOnHeader
				)
			and	vAllFiles.vHeaderFiles.none_of
				(	fDependsOnHeader
				)
			and	vAllFiles.vImplementationFiles.none_of
				(	fDependsOnHeader
				)
			)
		{
			cout << vHeaderOnly.SwapOut(nLeftOverIndex);
		}
		else
			++nLeftOverIndex;
	}

	cout << "\nHeader only file dependencies:\n";
	for	(	auto const
			&	rLeftOver
		:	vHeaderOnly
		)
	{
		cout << rLeftOver;
	}

	cout << "\nUnrelated files:\n";
	for	(	auto const
			&	rLeftOver
		:	vAllFiles.vHeaderFiles
		)
	{
		cout << rLeftOver;
	}

	cout << "\nUnrelated implementation files without header:\n";
	for	(	auto const
			&	rLeftOver
		:	vAllFiles.vImplementationFiles
		)
	{
		cout << rLeftOver;
	}
}
