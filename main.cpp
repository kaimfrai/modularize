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
	,	::std::find
		(	begin(i_rPaths)
		,	end(i_rPaths)
		,	i_rSwapOut
		)
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
		return i_rStream << i_rHeader.m_vPath << '\n';
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
		i_rStream << i_rHeader.m_vPath << '\n';
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
	->	path const&
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

	path const vSourceDir = EnsureDirectory(string_view{argv[1]}, "Source directory required as first argument!");
	path const vBinaryDir = EnsureDirectory(vSourceDir / string_view{argv[2]}, "Relative binary directory required as second argument!");
	path const vRootHeaderPath = EnsureHeader(vSourceDir / string_view{argv[3]}, "Relative header required as third argument!");


	FileStore
		vAllFiles
	{	vSourceDir
	,	vBinaryDir
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

	cout << "Required files:\n\n";
	cout << vRootHeader;

	::std::vector<HeaderFile>
		vDependentHeaders
	;

	auto const& rRootDependencies = vRootHeader.m_vImplementation.m_vDependency.m_vDependencies;
	vDependentHeaders.reserve(rRootDependencies.size());
	for	(	auto const
			&	rHeaderPath
		:	rRootDependencies
		)
	{
		vDependentHeaders.push_back(SwapOut(vAllFiles.vHeaderFiles.vFiles, HeaderFile{rHeaderPath}));
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
				nDependentIndex
			=	0uz
		;		nDependentIndex
			<	vDependentHeaders.size()
		;	// may erase in loop
		)
	{
		auto const& rDependentHeader = vDependentHeaders[nDependentIndex];
		if	(rDependentHeader.m_vPath.native().starts_with(vSourceDir.c_str()))
		{
			++nDependentIndex;
		}
		else
		{
			SwapOut(vDependentHeaders, begin(vDependentHeaders) + nDependentIndex);
		}
	}

	//	may grow over time
	for	(	auto
				nRequiredIndex
			=	0uz
		;		nRequiredIndex
			<	vRequiredHeaders.size()
		;	++	nRequiredIndex
		)
	{

		for	(	auto
					nDependentIndex
				=	0uz
			;		nDependentIndex
				<	vDependentHeaders.size()
			;	// may erase in loop
			)
		{
			//	memory location may change inside the loop
			auto const& rRequiredHeader = vRequiredHeaders[nRequiredIndex];
			auto const& rDependentHeader = vDependentHeaders[nDependentIndex];

			if	(rDependentHeader.IsHeaderOnly())
			{
				auto const vDependentHeader = SwapOut(vDependentHeaders, begin(vDependentHeaders) + nDependentIndex);
				// header only not handled
				if	(	vDependentHeader.m_vPath.native().starts_with(vSourceDir.c_str())
					and	(	end(vHeaderOnly)
						==	::std::find
							(	begin(vHeaderOnly)
							,	end(vHeaderOnly)
							,	vDependentHeader
							)
						)
					)
				{
					vHeaderOnly.push_back(::std::move(vDependentHeader));
				}
				continue;
			}

			if (not rDependentHeader.HasDependency())
			{
				cerr << "No dependency file found for " << rDependentHeader.m_vImplementation;
				SwapOut(vDependentHeaders, begin(vDependentHeaders) + nDependentIndex);
				continue;
			}

			auto const
			&	rDependentDependencies
			=	rDependentHeader.GetDependencies()
			;

			// not a cyclic dependency (yet)
			if	(	::std::none_of
					(	begin(rDependentDependencies)
					,	end(rDependentDependencies)
					,	[	&rRequiredHeader
						]	(	path const
								&	i_rDependentDependency
							)
						{	return i_rDependentDependency == rRequiredHeader.m_vPath;	}
					)
				)
			{
				++nDependentIndex;
				continue;
			}

			// add to required headers
			if	(	end(vRequiredHeaders)
				==	::std::find
					(	begin(vRequiredHeaders)
					,	end(vRequiredHeaders)
					,	rDependentHeader
					)
				)
			{
				vRequiredHeaders.push_back(rDependentHeader);
				cout << rDependentHeader;

				//	add dependencies of new required header
				for (	auto const
						&	rDependentDependency
					:	rDependentDependencies
					)

				{
					if	(	//	dont add headers in other directories
							rDependentDependency.native().starts_with(vSourceDir.c_str())
						and	(	end(vDependentHeaders)
							==	::std::find
								(	begin(vDependentHeaders)
								,	end(vDependentHeaders)
								,	rDependentDependency
								)
							)
						)
					{
						vDependentHeaders.push_back(SwapOut(vAllFiles.vHeaderFiles.vFiles, HeaderFile{rDependentDependency}));
					}
				}
			}
			// dependent header now in required headers
			SwapOut(vDependentHeaders, begin(vDependentHeaders) + nDependentIndex);
		}
	}

	cout << "\nDependent files:\n";
	for	(	auto const
			&	rLeftOver
		:	vDependentHeaders
		)
	{
		cout << rLeftOver;
	}

	cout << "\nDependent header only files:\n";
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

	cout << "\nImplementation files without header:\n";
	for	(	auto const
			&	rLeftOver
		:	vAllFiles.vImplementationFiles
		)
	{
		cout << rLeftOver;
	}
}
