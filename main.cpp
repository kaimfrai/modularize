#include "UniqueUnorderedVector.hpp"

#include <iostream>
#include <string_view>
#include <filesystem>
#include <queue>
#include <fstream>
#include <optional>
#include <compare>

using ::std::filesystem::path;
using ::std::string_view;
using ::std::cerr;
using ::std::cout;
using ::std::endl;
using ::std::queue;
using ::std::string;
using ::std::begin;
using ::std::end;

path SourceDir;
path BinaryDir;

auto constexpr
(	IsHeader
)	(	string_view
			i_vPath
	)
->	bool
{	return
	(	i_vPath.ends_with(".h")
	or	i_vPath.ends_with(".hpp")
	);
}

auto constexpr
(	IsImplementation
)	(	string_view
			i_vPath
	)
->	bool
{	return
	(	i_vPath.ends_with(".c")
	or	i_vPath.ends_with(".cpp")
	);
}

auto constexpr
(	IsDependency
)	(	string_view
			i_vPath
	)
->	bool
{	return
		i_vPath.ends_with(".o.d")
	;
}

using CheckFileFunc = auto(*)(string_view) -> bool;

template<typename t_tPath, CheckFileFunc t_fCheckFile>
struct
	Store
;

template
	<	typename
		...	t_tpPath
	,	CheckFileFunc
		...	t_fpCheckFile
	>
auto
(	PopulateFiles
)	(	path const
		&	i_rSourceDir
	,	Store<t_tpPath, t_fpCheckFile>
		&
		...	i_rStore
	)
->	void
{
	::std::filesystem::recursive_directory_iterator const
		vSourceFiles
	{	i_rSourceDir
	};

	for	(	auto const
		&	i_rEntry
		:	vSourceFiles
		)
	{
		if	(not i_rEntry.is_regular_file())
			continue;

		auto const
		&	rPath
		=	i_rEntry.path()
		;

		(void)
		(	...
		or	i_rStore.AddEntry(rPath)
		);
	}
}

struct
	DepFile
{
	path m_vPath;
	path m_vImplementation;
	Modularize::UniqueUnorderedVector<path>
		m_vDependencies
	;

	friend auto inline
	(	operator <=>
	)	(	DepFile const
			&	i_rLeft
		,	DepFile const
			&	i_rRight
		)
	{
		return i_rLeft.m_vPath <=> i_rRight.m_vPath;
	}

	static auto
	(	ReadEntry
	)	(	::std::stringstream
			&	i_rLine
		)
	->	path
	{
		string
			vPath
		;
		while(getline(i_rLine, vPath, ' '))
		{
			if	(vPath.empty() or vPath[0] == '\\')
				continue;

			return path{vPath};
		}
		return path{};
	}

	DepFile() = default;
	DepFile(path const& i_rPath)
	:	m_vPath{i_rPath}
	{
		::std::ifstream
			vFile = i_rPath
		;
		string
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
						cerr << "\nFound more than one implementation in " << m_vPath << '\n';
					else
						m_vImplementation = vPath;
				}
				else
					m_vDependencies.push_back(vPath);
			}
		}
	}
};

struct
	ImplementationFile
{
	path m_vPath;
	DepFile m_vDependency;

	bool HasDependency() const
	{
		return not m_vDependency.m_vPath.empty();
	}

	auto GetDependencies() const
	->	Modularize::UniqueUnorderedVector<path> const&
	{
		return m_vDependency.m_vDependencies;
	}

	ImplementationFile() = default;

	explicit(false)
	(	ImplementationFile
	)	(	path const
			&	i_rPath
		)
	:	m_vPath
		{	i_rPath
		}
	{}

	auto
	(	SetDependency
	)	(	Modularize::UniqueUnorderedVector<DepFile>
			&	i_rDependencyFiles
		)
	->	bool
	{
		auto const
			vPosition
		=	i_rDependencyFiles.find_if
			(	[&]	(	DepFile const
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

		if	(vPosition == end(i_rDependencyFiles))
			return false;

		m_vDependency = i_rDependencyFiles.SwapOut(vPosition);
		return true;
	}

	friend auto
	(	operator<=>
	)	(	ImplementationFile const
			&	i_rLeft
		,	ImplementationFile const
			&	i_rRight
		)
	{
		return i_rLeft.m_vPath <=> i_rRight.m_vPath;
	}

	friend auto
	(	operator <<
	)	(	auto
			&	i_rStream
		,	ImplementationFile const
			&	i_rHeader
		)
	->	decltype(i_rStream)
	{
		return i_rStream << i_rHeader.m_vPath.lexically_relative(SourceDir) << '\n';
	}
};

static auto
(	SearchImplementation
)	(	path
			i_vBase
	,	Modularize::UniqueUnorderedVector<ImplementationFile>
		&	i_rImplementationFiles
	)
->	typename Modularize::UniqueUnorderedVector<ImplementationFile>::iterator
{
	i_vBase.replace_extension();
	string
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
			path vNoExt = i_rPath.m_vPath;
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
	{	string
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
				path vNoExt = i_rPath.m_vPath;
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
				path vNoExt = i_rPath.m_vPath;
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
	path m_vPath;
	ImplementationFile m_vImplementation;

	bool IsHeaderOnly() const
	{
		return m_vImplementation.m_vPath.empty();
	}

	bool HasDependency() const
	{
		return m_vImplementation.HasDependency();
	}

	auto GetDependencies() const
	->	Modularize::UniqueUnorderedVector<path> const&
	{
		return m_vImplementation.GetDependencies();
	}

	friend auto
	(	operator<=>
	)	(	HeaderFile const
			&	i_rLeft
		,	HeaderFile const
			&	i_rRight
		)
	{
		return i_rLeft.m_vPath <=> i_rRight.m_vPath;
	}

	friend auto
	(	operator ==
	)	(	HeaderFile const
			&	i_rLeft
		,	HeaderFile const
			&	i_rRight
		)
	->	bool
	{
		return i_rLeft.m_vPath == i_rRight.m_vPath;
	}

	HeaderFile() = default;
	explicit(false)
	(	HeaderFile
	)	(	path const
			&	i_rPath
		)
	:	m_vPath{i_rPath}
	{}

	bool SetImplementation(Modularize::UniqueUnorderedVector<ImplementationFile>& i_rImplementationFiles)
	{
		auto vImplementationIt = SearchImplementation(m_vPath, i_rImplementationFiles);
		if	(vImplementationIt == i_rImplementationFiles.end())
			return false;

		m_vImplementation = i_rImplementationFiles.SwapOut(vImplementationIt);
		return true;
	}

	auto
	(	SetDependency
	)	(	Modularize::UniqueUnorderedVector<DepFile>
			&	i_rDependencyFiles
		)
	->	bool
	{
		return m_vImplementation.SetDependency(i_rDependencyFiles);
	}

	friend auto
	(	operator <<
	)	(	auto
			&	i_rStream
		,	HeaderFile const
			&	i_rHeader
		)
	->	decltype(i_rStream)
	{

		i_rStream << i_rHeader.m_vPath.lexically_relative(SourceDir) << '\n';
		if	(	not
				i_rHeader.IsHeaderOnly()
			)
			i_rStream << i_rHeader.m_vImplementation;
		return i_rStream;
	}
};

template<typename t_tPath, CheckFileFunc t_fCheckFile>
struct
	Store
:	Modularize::UniqueUnorderedVector<t_tPath>
{
	Store() = default;

	explicit(true)
	(	Store
	)	(	path const
			&	i_rSourceDir
		)
	{
		::PopulateFiles(i_rSourceDir, *this);
	}

	auto AddEntry(path const& i_rEntry) &
	{
		bool const bAdd = t_fCheckFile(i_rEntry.c_str());
		if	(bAdd)
			this->push_back(i_rEntry);
		return bAdd;
	}
};

using HeaderStore = Store<HeaderFile, &IsHeader>;
using ImplementationStore = Store<ImplementationFile, &IsImplementation>;
using DependencyStore = Store<DepFile, &IsDependency>;

struct
	FileStore
{
	HeaderStore
		vHeaderFiles
	;

	ImplementationStore
		vImplementationFiles
	;

	DependencyStore
		vDependencyFiles
	;

	explicit(true)
	(	FileStore
	)	(	path const
			&	i_rSourceDir
		,	path const
			&	i_rBinaryDir
		)
	{
		::PopulateFiles(i_rSourceDir, vHeaderFiles, vImplementationFiles);
		::PopulateFiles(i_rBinaryDir, vDependencyFiles);
		for	(	auto
				&	rHeader
			:	vHeaderFiles
			)
		{
			if	(rHeader.SetImplementation(vImplementationFiles))
				rHeader.SetDependency(vDependencyFiles);
		}
		for	(	auto
				&	rImplementation
			:	vImplementationFiles
			)
		{
			rImplementation.SetDependency(vDependencyFiles);
		}
	}
};

auto
(	EnsureDirectory
)	(	path const
		&	i_rPath
	,	string_view
			i_sErrorMessage
	)
->	path
{
	if (not exists(i_rPath) or not is_directory(i_rPath))
	{
		cerr << i_sErrorMessage << endl;
		::std::exit(EXIT_FAILURE);
	}
	return i_rPath;
}

auto
(	EnsureHeader
)	(	path const
		&	i_rPath
	,	string_view
			i_sErrorMessage
	)
->	path
{
	if (not exists(i_rPath) or not IsHeader(i_rPath.c_str()))
	{
		cerr << i_sErrorMessage << endl;
		::std::exit(EXIT_FAILURE);
	}
	return i_rPath;
}



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
		cerr << "Path to source directory, binary directory, and relative header required as arguments!" << endl;
		return EXIT_FAILURE;
	}

	SourceDir = EnsureDirectory(string_view{argv[1]}, "Source directory required as first argument!");
	BinaryDir = EnsureDirectory(SourceDir / string_view{argv[2]}, "Relative binary directory required as second argument!");
	path const vRootHeaderPath = EnsureHeader(SourceDir / string_view{argv[3]}, "Relative header required as third argument!");


	FileStore
		vAllFiles
	{	SourceDir
	,	BinaryDir
	};
	auto const vRootHeader = vAllFiles.vHeaderFiles.SwapOut(HeaderFile{vRootHeaderPath});

	if	(vRootHeader.IsHeaderOnly())
	{
		cerr << "Could not find an implementation file for " << vRootHeader << endl;
		::std::exit(EXIT_FAILURE);
	}
	if	(not vRootHeader.HasDependency())
	{
		cerr << "Could not find a dependency file for " << vRootHeader.m_vImplementation << endl;
		::std::exit(EXIT_FAILURE);
	}

	cout << "Required files:\n";
	cout << vRootHeader;

	Modularize::UniqueUnorderedVector<HeaderFile>
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
			(	HeaderFile{rHeaderPath}
			)
		);
	}

	Modularize::UniqueUnorderedVector<HeaderFile>
		vRequiredHeaders
	;
	vRequiredHeaders.push_back(vRootHeader);

	Modularize::UniqueUnorderedVector<HeaderFile>
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
		if	(rDependentOnHeader.m_vPath.native().starts_with(SourceDir.c_str()))
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
				if	(	vDependentOnHeader.m_vPath.native().starts_with(SourceDir.c_str())
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
						]	(	path const
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
						rDependentOnDependency.native().starts_with(SourceDir.c_str())
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
						(	HeaderFile{rDependentOnDependency}
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

	Modularize::UniqueUnorderedVector<HeaderFile>
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
			]	(	HeaderFile const
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
					]	(	HeaderFile const
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
