#include <iostream>
#include <string_view>
#include <filesystem>
#include <vector>
#include <queue>
#include <fstream>
#include <optional>
#include <compare>

using ::std::filesystem::path;
using ::std::string_view;
using ::std::cerr;
using ::std::cout;
using ::std::endl;
using ::std::vector;
using ::std::queue;
using ::std::string;
using ::std::begin;
using ::std::end;

path SourceDir;
path BinaryDir;

template
	<	typename
			t_tContainer
	>
auto inline
(	Find
)	(	t_tContainer
		&	i_rContainer
	,	typename t_tContainer::value_type const
		&	i_rValue
	)
{	return
	::std::find
	(	begin(i_rContainer)
	,	end(i_rContainer)
	,	i_rValue
	);
}

template
	<	typename
			t_tContainer
	>
auto inline
(	Contains
)	(	t_tContainer const
		&	i_rContainer
	,	typename t_tContainer::value_type const
		&	i_rValue
	)
->	bool
{
	auto const
		vBegin
	=	begin(i_rContainer)
	;
	auto const
		vEnd
	=	end(i_rContainer)
	;
	return
		vEnd
	!=	::std::find
		(	vBegin
		,	vEnd
		,	i_rValue
		)
	;
}

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

template
	<	typename
			t_tPath
	>
auto inline
(	SwapOut
)	(	vector<t_tPath>
		&	i_rPaths
	,	vector<t_tPath>::iterator
			i_vPosition
	)
->	t_tPath
{
	if	(i_vPosition != end(i_rPaths))
	{
		using ::std::swap;
		swap(*i_vPosition, i_rPaths.back());
		auto const vResult = ::std::move(i_rPaths.back());
		i_rPaths.pop_back();
		return vResult;
	}
	else
		return {};
}

template<typename t_tPath>
auto inline
(	SwapOut
)	(	vector<t_tPath>
		&	i_rPaths
	,	t_tPath const
		&	i_rSwapOut
	)
->	t_tPath
{
	return
	SwapOut
	(	i_rPaths
	,	Find
		(	i_rPaths
		,	i_rSwapOut
		)
	);
}

template<typename t_tPath>
auto inline
(	SwapOutByIndex
)	(	vector<t_tPath>
		&	i_rPaths
	,	::std::size_t
			i_nIndex
	)
->	t_tPath
{
	return
	SwapOut
	(	i_rPaths
	,	begin(i_rPaths)
	+	i_nIndex
	);
}

template
	<	typename
			t_tIterator
	,	typename
		...	t_tpPredicates
	>
auto
(	FindByPriority
)	(	t_tIterator
			i_vBegin
	,	t_tIterator
			i_vEnd
	,	t_tpPredicates const
		&
		...	i_rpPredicates
	)
->	t_tIterator
{
	t_tIterator
		vPosition
	=	i_vEnd
	;
	(void)(	...
	or	(	(	vPosition
			=	::std::find_if
				(	i_vBegin
				,	i_vEnd
				,	i_rpPredicates
				)
			)
		!=	i_vEnd
		)
	);
	return vPosition;
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
	::std::vector<path>
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
		// second line is implementation
		getline(vFile, vLine);
		::std::stringstream
			vColumn
		{	vLine
		};
		m_vImplementation = ReadEntry(vColumn);

		while(getline(vFile, vLine))
		{
			vColumn.str(vLine);
			auto const
				vPath
			=	ReadEntry
				(	vColumn
				)
			;
			if	(not vPath.empty())
				m_vDependencies.push_back(vPath);
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
	->	vector<path> const&
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
	)	(	::std::vector<DepFile>
			&	i_rDependencyFiles
		)
	->	bool
	{
		auto const
			vPosition
		=	::std::find_if
			(	begin(i_rDependencyFiles)
			,	end(i_rDependencyFiles)
			,	[&]	(	DepFile const
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

		m_vDependency = SwapOut(i_rDependencyFiles, vPosition);
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
	,	vector<ImplementationFile>::iterator
			i_vBegin
	,	vector<ImplementationFile>::iterator
			i_vEnd
	)
->	vector<ImplementationFile>::iterator
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
		FindByPriority
		(	i_vBegin
		,	i_vEnd
		,	fPriority1
		,	fPriority2
		,	fPriority3
		);
	}
	else
		return
		FindByPriority
		(	i_vBegin
		,	i_vEnd
		,	fPriority1
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
	->	vector<path> const&
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

	bool SetImplementation(::std::vector<ImplementationFile>& i_rImplementationFiles)
	{
		auto vImplementationIt = SearchImplementation(m_vPath, begin(i_rImplementationFiles), end(i_rImplementationFiles));
		if	(vImplementationIt == end(i_rImplementationFiles))
			return false;

		m_vImplementation = SwapOut(i_rImplementationFiles, vImplementationIt);
		return true;
	}

	auto
	(	SetDependency
	)	(	::std::vector<DepFile>
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
{
	vector<t_tPath>
		vFiles
	;
	Store() = default;

	explicit(true)
	(	Store
	)	(	path const
			&	i_rSourceDir
		)
	{
		::PopulateFiles(i_rSourceDir, vFiles);
	}

	operator vector<t_tPath>&
		()	&
	{	return vFiles;	}

	auto
	(	operator[]
	)	(	::std::size_t
				i_nIndex
		)	const&
	->	t_tPath const&
	{
		return vFiles[i_nIndex];
	}

	auto
	(	size
	)	()	const
	{
		return vFiles.size();
	}

	auto
	(	begin
	)	()
	{
		return vFiles.begin();
	}

	auto
	(	end
	)	()
	{
		return vFiles.end();
	}

	auto AddEntry(path const& i_rEntry)
	{
		bool const bAdd = t_fCheckFile(i_rEntry.c_str());
		if	(bAdd)
			vFiles.push_back(i_rEntry);
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
	auto const vRootHeader = SwapOut(vAllFiles.vHeaderFiles.vFiles, HeaderFile{vRootHeaderPath});

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

	::std::vector<HeaderFile>
		vDependentOnHeaders
	;

	auto const& rRootDependencies = vRootHeader.m_vImplementation.m_vDependency.m_vDependencies;
	vDependentOnHeaders.reserve(rRootDependencies.size());
	for	(	auto const
			&	rHeaderPath
		:	rRootDependencies
		)
	{
		vDependentOnHeaders.push_back(SwapOut(vAllFiles.vHeaderFiles.vFiles, HeaderFile{rHeaderPath}));
	}

	::std::vector<HeaderFile>
		vRequiredHeaders
	;
	vRequiredHeaders.push_back(vRootHeader);

	::std::vector<HeaderFile>
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
			SwapOut(vDependentOnHeaders, begin(vDependentOnHeaders) + nDependentOnIndex);
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
				auto const vDependentOnHeader = SwapOut(vDependentOnHeaders, begin(vDependentOnHeaders) + nDependentOnIndex);
				// header only not handled
				if	(	vDependentOnHeader.m_vPath.native().starts_with(SourceDir.c_str())
					and	(	not
							Contains
							(	vHeaderOnly
							,	vDependentOnHeader
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
				SwapOut(vDependentOnHeaders, begin(vDependentOnHeaders) + nDependentOnIndex);
				continue;
			}

			auto const
			&	rDependentOnDependencies
			=	rDependentOnHeader.GetDependencies()
			;

			// not a cyclic dependency (yet)
			if	(	::std::none_of
					(	begin(rDependentOnDependencies)
					,	end(rDependentOnDependencies)
					,	[	&rRequiredHeader
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
						Contains
						(	vDependentOnHeaders
						,	rDependentOnDependency
						)
					and	not
						Contains
						(	vRequiredHeaders
						,	rDependentOnDependency
						)
					)
				{
					vDependentOnHeaders.push_back(SwapOut(vAllFiles.vHeaderFiles.vFiles, HeaderFile{rDependentOnDependency}));
				}
			}

			// add to required headers
			cout << rDependentOnHeader;
			vRequiredHeaders.push_back( SwapOutByIndex(vDependentOnHeaders, nDependentOnIndex));

			// restart loop accounting for new required header
			nRequiredIndex = 0;
			break;
		}
	}

	cout << "\nFile dependencies:\n";
	for	(	auto const
			&	rLeftOver
		:	vDependentOnHeaders
		)
	{
		cout << rLeftOver;
	}


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
		if	(	::std::any_of
				(	begin(vRequiredHeaders)
				,	end(vRequiredHeaders)
				,	[	&rDependencies
					]	(	HeaderFile const
							&	i_rRequired
						)
					{
						return Contains(rDependencies, i_rRequired.m_vPath);
					}
				)
			)
		{
			cout << SwapOutByIndex(vAllFiles.vHeaderFiles.vFiles, nLeftOverIndex);
		}
		else
			++nLeftOverIndex;
	}

	cout << "\nInverse implementation files without header dependencies:\n";
	for	(	auto
				nLeftOverIndex
			=	0uz
		;		nLeftOverIndex
			<	vAllFiles.vImplementationFiles.size()
		;
		)
	{
		auto const& rDependencies = vAllFiles.vImplementationFiles[nLeftOverIndex].GetDependencies();
		if	(	::std::any_of
				(	begin(vRequiredHeaders)
				,	end(vRequiredHeaders)
				,	[	&rDependencies
					]	(	HeaderFile const
							&	i_rRequired
						)
					{
						return Contains(rDependencies, i_rRequired.m_vPath);
					}
				)
			)
		{
			cout << SwapOutByIndex(vAllFiles.vImplementationFiles.vFiles, nLeftOverIndex);
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
				return Contains(i_rLeftOverFile.GetDependencies(), vHeaderPath);
			}
		;

		if	(	::std::none_of
				(	begin(vDependentOnHeaders)
				,	end(vDependentOnHeaders)
				,	fDependsOnHeader
				)
			and	::std::none_of
				(	begin(vAllFiles.vHeaderFiles)
				,	end(vAllFiles.vHeaderFiles)
				,	fDependsOnHeader
				)
			and	::std::none_of
				(	begin(vAllFiles.vImplementationFiles)
				,	end(vAllFiles.vImplementationFiles)
				,	fDependsOnHeader
				)
			)
		{
			cout << SwapOutByIndex(vHeaderOnly, nLeftOverIndex);
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

	cout << "\nLeftover files:\n";
	for	(	auto const
			&	rLeftOver
		:	vAllFiles.vHeaderFiles
		)
	{
		cout << rLeftOver;
	}

	cout << "\nLeftover implementation files without header:\n";
	for	(	auto const
			&	rLeftOver
		:	vAllFiles.vImplementationFiles
		)
	{
		cout << rLeftOver;
	}
}
