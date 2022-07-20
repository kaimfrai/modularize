#include "ModuleInterfaceConverter.hpp"

#include "HeaderFile.hpp"
#include "UnorderedVector.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>
#include <map>
#include <string>

static std::string_view constexpr ImportableHeaders[]
{	"algorithm"
,	"any"
,	"array"
,	"atomic"
,	"barrier"
,	"bit"
,	"bitset"
,	"charconv"
,	"chrono"
,	"codecvt"
,	"compare"
,	"complex"
,	"concepts"
,	"condition_variable"
,	"coroutine"
,	"deque"
,	"exception"
,	"execution"
,	"expected"
,	"filesystem"
,	"format"
,	"forward_list"
,	"fstream"
,	"functional"
,	"future"
,	"initializer_list"
,	"iomanip"
,	"ios"
,	"iosfwd"
,	"iostream"
,	"istream"
,	"iterator"
,	"latch"
,	"limits"
,	"list"
,	"locale"
,	"map"
,	"memory"
,	"memory_resource"
,	"mutex"
,	"new"
,	"numbers"
,	"numeric"
,	"optional"
,	"ostream"
,	"queue"
,	"random"
,	"ranges"
,	"ratio"
,	"regex"
,	"scoped_allocator"
,	"semaphore"
,	"set"
,	"shared_mutex"
,	"source_location"
,	"span"
,	"spanstream"
,	"sstream"
,	"stack"
,	"stacktrace"
,	"stdexcept"
,	"stop_token"
,	"streambuf"
,	"string"
,	"string_view"
,	"strstream"
,	"syncstream"
,	"system_error"
,	"thread"
,	"tuple"
,	"typeindex"
,	"typeinfo"
,	"type_traits"
,	"unordered_map"
,	"unordered_set"
,	"utility"
,	"valarray"
,	"variant"
,	"vector"
,	"version"
};

auto
(	::Modularize::ModuleInterfaceConverter::FlushPending
)	(	::std::stringstream
		&	i_rTargetFragment
	)
->	::std::stringstream&
{
	i_rTargetFragment << m_vPending.str();
	m_vPending.clear();
	m_vPending.str(::std::string{});
	return i_rTargetFragment;
}

auto
(	::Modularize::ModuleInterfaceConverter::ParseLine
)	(	UnorderedVector<ModuleInterface> const
		&	i_rModuleInterfaces
	,	::std::string_view
			i_vLine
	,	::std::stringstream
		&	i_rCurrentFragment
	)
->	::std::stringstream*
{
	::std::string_view
		sNoWhitespace
	=	i_vLine
	;
	if	(	auto const
				nCharacterPos
			=	i_vLine.find_first_not_of(" \t")
		;	nCharacterPos
		!=	::std::string_view::npos
		)
		sNoWhitespace.remove_prefix(nCharacterPos);

	//	full-line comments go to the next fragment
	if	(	sNoWhitespace.starts_with("//")
		)
	{
		m_vPending << i_vLine << '\n';
		return &i_rCurrentFragment;
	}

	//	block comments go to the next fragment but require more inspection
	::std::size_t const
		nOldBlockCommentCounter
	=	m_nBlockCommentCounter
	;
	::std::size_t const
		nOldNestedScopeCounter
	=	m_nNestedScopeCounter
	;

	for	(	char
				vLastChar
			=	'\n'
		;	char
				vCurrentChar
		:	i_vLine
		)
	{
			m_nBlockCommentCounter
		+=	(	vLastChar == '/'
			and	vCurrentChar == '*'
			)
		;

			m_nBlockCommentCounter
		-=	(	vLastChar == '*'
			and	vCurrentChar == '/'
			)
		;

		//	change scope only outside a comment
			m_nNestedScopeCounter
		+=	(	m_nBlockCommentCounter == 0uz
			and	vCurrentChar == '{'
			)
		;
			m_nNestedScopeCounter
		-=	(	m_nBlockCommentCounter == 0uz
			and	vCurrentChar == '}'
			)
		;


		vLastChar = vCurrentChar;
	}

	//	started in a block comment or started a new block comment
	if	(	m_nBlockCommentCounter > 0uz
		or	nOldBlockCommentCounter > 0uz
		)
	{
		m_vPending	<< i_vLine << '\n';
		return &i_rCurrentFragment;
	}

	//	skip new lines ouside of scopes
	if	(	sNoWhitespace.empty()
		and	m_nNestedScopeCounter == 0uz
		)
		return &i_rCurrentFragment;

	if	(sNoWhitespace.starts_with("#"))
	{
		::std::string_view const
			sPPDirective
		{	sNoWhitespace.begin() + sNoWhitespace.find_first_not_of(" \t", 1uz)
		,	sNoWhitespace.end()
		};

		if	(	sPPDirective.starts_with("if")
			)
		{
			++m_nNestedPPDirectiveCounter;

			// skip over opening header guard
			if	(	&i_rCurrentFragment == &m_vHeadComment
				and	sPPDirective.starts_with("ifndef")
				)
			{
				m_sHeaderGuard.assign
				(	sPPDirective.begin() + sPPDirective.find_first_not_of(" \t", sizeof("ifndef"))
				,	sPPDirective.end()
				);

				// comments before header guard are interpreted as head commment
				FlushPending(m_vHeadComment);

				return &m_vGlobalFragment;
			}

			// Not Header Guard
			m_vPending << i_vLine << '\n';
			return &i_rCurrentFragment;
		}

		if	(	sPPDirective.starts_with("endif")
			)
		{
			--m_nNestedPPDirectiveCounter;
			//	skip over closing header guard
			if	(	m_nNestedPPDirectiveCounter == 0uz
				and	not m_sHeaderGuard.empty()
				)
			{
				m_sHeaderGuard.clear();
				return &i_rCurrentFragment;
			}

			//	not closing header guard
			FlushPending(i_rCurrentFragment) << i_vLine << '\n';
			return &i_rCurrentFragment;
		}

		if	(	sPPDirective.starts_with("pragma")
			)
		{
			::std::string_view const
				sPragma
			{	sPPDirective.begin() + sPPDirective.find_first_not_of(" \t", sizeof("pragma"))
			,	sPPDirective.end()
			};
			//	skip only #pragma once
			if	(	sPragma.starts_with("once")
				)
				// comments before pragma once are interpreted as head commment
				FlushPending(m_vHeadComment);
			else
				m_vPending << i_vLine << '\n';

			return &i_rCurrentFragment;
		}

		if	(	sPPDirective.starts_with("define")
			)
		{
			::std::string_view const
				sDefinition
			{	sPPDirective.begin() + sPPDirective.find_first_not_of(" \t", sizeof("define"))
			,	sPPDirective.end()
			};

			//	skip only the designated header guard
			if	(	m_sHeaderGuard.empty()
				or	not
					sDefinition.starts_with
					(	m_sHeaderGuard
					)
				)
				m_vPending << i_vLine << '\n';

			return &i_rCurrentFragment;
		}

		if	(	sPPDirective.starts_with("undef")
			)
		{
			// expect undef to be as close to the #define as possible
			FlushPending(i_rCurrentFragment) << i_vLine << '\n';
			return &i_rCurrentFragment;
		}

		if	(	sPPDirective.starts_with("include")
			)
		{
			::std::string_view const
				sInclude
			{	// ignore relative paths too
				sPPDirective.begin() + sPPDirective.find_first_not_of(" \t.\\/\"<", sizeof("include"))
			,	sPPDirective.begin() + sPPDirective.find_last_of(">\"")
			};

			//	standard header can be imported
			if	(	auto const vImportableHeaderIt
					=	std::ranges::find
						(	ImportableHeaders
						,	sInclude
						)
				;	(	vImportableHeaderIt
					!=	end(ImportableHeaders)
					)
				)
			{
				//	first comment always head comment
				FlushPending(m_vHeadComment.str().empty() ? m_vHeadComment : m_vStandardComment);
				if	(	auto
							vInsertPosition
						=	::std::lower_bound
							(	m_vStandardImports.begin()
							,	m_vStandardImports.end()
							,	sInclude
							)
					;	vInsertPosition == m_vStandardImports.end()
					or	*vInsertPosition != sInclude
					)
				{
					m_vStandardImports.emplace(vInsertPosition, sInclude.begin(), sInclude.end());
				}

				return &m_vNamedFragment;
			}

			std::filesystem::path
				vFullIncludePath
			=	sInclude
			;

			//	resolve full path first
			//	there could be multiple files with the same name in different directories
			if	(	auto const
						vPathIt
					=	m_rDependencies.find_if
						(	[	sInclude
							]	(	::std::filesystem::path const
									&	i_rDependencyPath
								)
							->	bool
							{	return
								(	i_rDependencyPath.filename().native()
								==	sInclude
								);
							}
						)
				;	vPathIt
				!=	m_rDependencies.end()
				)
			{
				vFullIncludePath = *vPathIt;
			}

			auto const
				vImportedModuleInterfaceIt
			=	[	&i_rModuleInterfaces
				,	&rDependencies = this->m_rDependencies
				,	&sInclude
				]{
					//	attempt to resolve full path first
					//	there could be multiple files with the same name in different directories
					if	(	auto const
								vPathIt
							=	rDependencies.find_if
								(	[	sInclude
									]	(	::std::filesystem::path const
											&	i_rDependencyPath
										)
									->	bool
									{	return
											i_rDependencyPath.native()
										.	ends_with(sInclude)
										;
									}
								)
						;	vPathIt
						!=	rDependencies.end()
						)
					{
						return
						i_rModuleInterfaces.find_if
						(	[	&vFullIncludePath = *vPathIt
							]	(	ModuleInterface const
									&	i_rModuleInterface
								)
							->	bool
							{	return
									i_rModuleInterface.m_vInterface.m_vPath
								==	vFullIncludePath
								;
							}
						);
					}
					else
					{
						// fallback on simple suffix
						return
						i_rModuleInterfaces.find_if
						(	[	&sInclude
							]	(	ModuleInterface const
									&	i_rModuleInterface
								)
							->	bool
							{	return
									i_rModuleInterface.m_vInterface.m_vPath.native()
								.	ends_with(sInclude)
								;
							}
						);
					}
				}()
			;

			if	(	vImportedModuleInterfaceIt
				!=	i_rModuleInterfaces.end()
				)
			{
				if	(	not
						vImportedModuleInterfaceIt->m_sPartitionName.empty()
					and	(	vImportedModuleInterfaceIt->m_sModuleName
						==	m_sModuleName
						)
					)
				{
					//	first comment always head comment
					FlushPending(m_vHeadComment.str().empty() ? m_vHeadComment : m_vPartitionComment);
					if	(	vImportedModuleInterfaceIt->m_sPartitionName
						==	m_sPartitionName
						)
					{
						//	no import of self
						return &m_vNamedFragment;
					}
					else
					if	(	auto
								vInsertPosition
							=	::std::lower_bound
								(	m_vPartitionImports.begin()
								,	m_vPartitionImports.end()
								,	vImportedModuleInterfaceIt->m_sPartitionName
								)
						;	vInsertPosition == m_vPartitionImports.end()
						or	*vInsertPosition != vImportedModuleInterfaceIt->m_sPartitionName
						)
					{
						m_vPartitionImports.insert(vInsertPosition, vImportedModuleInterfaceIt->m_sPartitionName);
					}
				}
				else
				{
					//	first comment always head comment
					FlushPending(m_vHeadComment.str().empty() ? m_vHeadComment : m_vImportComment);
					if	(	auto
								vInsertPosition
							=	::std::lower_bound
								(	m_vPureImports.begin()
								,	m_vPureImports.end()
								,	vImportedModuleInterfaceIt->m_sModuleName
								)
						;	vInsertPosition == m_vPureImports.end()
						or	*vInsertPosition != vImportedModuleInterfaceIt->m_sModuleName
						)
					{
						m_vPureImports.insert(vInsertPosition, vImportedModuleInterfaceIt->m_sModuleName);
					}
				}

				return &m_vNamedFragment;
			}

			//	first comment always head comment
			FlushPending(m_vHeadComment.str().empty() ? m_vHeadComment : m_vGlobalFragment);
			//	no module interface found => global header
			m_vGlobalFragment << i_vLine << '\n';
			return &m_vGlobalFragment;
		}

		//	bundle all other directives together with next fragment
		m_vPending << i_vLine << '\n';
		return &i_rCurrentFragment;
	}

	//	global entity needs to be exported
	if	(	nOldNestedScopeCounter == 0uz
		)
	{
		if	(	sNoWhitespace.starts_with("static")
			)
		{
			//	do not export static
			FlushPending(m_vNamedFragment) << i_vLine << '\n';
			return &m_vNamedFragment;
		}

		//	heuristic for global entities
		if	(	sNoWhitespace.starts_with("namespace")
			or	sNoWhitespace.starts_with("typedef")
			or	sNoWhitespace.starts_with("using")
			or	sNoWhitespace.starts_with("class")
			or	sNoWhitespace.starts_with("struct")
			or	sNoWhitespace.starts_with("union")
			or	sNoWhitespace.starts_with("enum")
			or	sNoWhitespace.starts_with("template")
			or	sNoWhitespace.starts_with("const")
			or	sNoWhitespace.starts_with("constexpr")
			or	sNoWhitespace.starts_with("inline")
			or	sNoWhitespace.starts_with("auto")
			or	sNoWhitespace.starts_with("void")
			or	sNoWhitespace.starts_with("bool")
			or	sNoWhitespace.starts_with("char")
			or	sNoWhitespace.starts_with("short")
			or	sNoWhitespace.starts_with("int")
			or	sNoWhitespace.starts_with("float")
			or	sNoWhitespace.starts_with("double")
			or	sNoWhitespace.starts_with("long")
			or	sNoWhitespace.starts_with("unsigned")
			or	sNoWhitespace.starts_with("signed")
			or	sNoWhitespace.starts_with("std::")
			or	sNoWhitespace.starts_with("::std::")
			or	// export required before attribute
				sNoWhitespace.starts_with("[[")
			)
		{
			m_vNamedFragment << '\n';
			(	FlushPending(m_vNamedFragment)
			<<	m_sExportEntity
			<< i_vLine << '\n'
			);
			return &m_vNamedFragment;
		}

		if	(sNoWhitespace.starts_with("extern"))
		{
			(	FlushPending(m_vGlobalFragment)
			<<	i_vLine << '\n'
			);
			return &m_vGlobalFragment;
		}
	}

	FlushPending(i_rCurrentFragment) << i_vLine << '\n';
	return &i_rCurrentFragment;
}

auto
(	::Modularize::ModuleInterfaceConverter::ReadModule
)	(	UnorderedVector<ModuleInterface> const
		&	i_rModuleInterfaces
	)
->	void
{
	::std::ifstream
		vContent
	{	m_rFilePath
	};

	::std::string
		sLine
	;

	::std::stringstream
	*	aCurrent
	=	&m_vHeadComment
	;

	while
		(	getline
			(	vContent
			,	sLine
			)
		)
	{
		aCurrent
		=	ParseLine
			(	i_rModuleInterfaces
			,	sLine
			,	*aCurrent
			)
		;
	}

	FlushPending(m_vNamedFragment);
}

auto
(	::Modularize::ModuleInterfaceConverter::WriteModule
)	()
->	void
{
	::std::ofstream
		vWriter
	{	m_rFilePath
	};

	if	(	not
				m_vHeadComment.str()
			.	empty
				()
		)
	{
			vWriter
		<<	m_vHeadComment.str()
		//	no new line between head comment and first fragment
// 		<<	'\n'
		;
	}

	if	(	not
				m_vGlobalFragment.str()
			.	empty
				()
		)
	{		vWriter
		<<	"module;\n\n"
		<<	m_vGlobalFragment.str()
		//	new line between global and named fragment
		<<	'\n';

	}

	//	write named module fragment

	(	vWriter
	<<	m_sExportModule
	<<	m_sModuleName
	);
	if	(	not
			m_sPartitionName.empty()
		)
		(	vWriter
		<<	':'
		<<	m_sPartitionName
		);
	(	vWriter
	<<	';'
	<<	'\n'
	);

	if	(	not
			m_vPartitionImports.empty()
		)
	{
		vWriter << '\n';
		vWriter << m_vPartitionComment.str();
		for	(	auto const
				&	sPartitionName
			:	m_vPartitionImports
			)
		{
			vWriter << m_sExportImport << ':' << sPartitionName << ';' << '\n';
		}
	}

	if	(	not
			m_vPureImports.empty()
		)
	{
		vWriter << '\n';
		vWriter << m_vImportComment.str();
		for	(	auto const
				&	sImportName
			:	m_vPureImports
			)
		{
			vWriter << m_sExportImport << sImportName << ';' << '\n';
		}
	}

	if	(	not
			m_vStandardImports.empty()
		)
	{
		vWriter << '\n';
		vWriter << m_vImportComment.str();
		for	(	auto const
				&	sImportName
			:	m_vStandardImports
			)
		{
			// always use <> for standard imports
			vWriter << m_sExportImport << '<' << sImportName << '>' << ';' << '\n';
		}
	}

	if	(	m_vNamedFragment.str()[0uz]
		!=	'\n'
		)
		vWriter << '\n';
	vWriter << m_vNamedFragment.str();
}

auto
(	::Modularize::Modularize
)	(	::std::span<char const*>
			i_vArgument
	)
->	void
{
	if	(	i_vArgument.size()
		<	3uz
		)
	{
		::std::cerr << "Path to source directory, binary directory, and root module name required as arguments!" << ::std::endl;
		::std::exit(EXIT_FAILURE);
	}

	Directory const SourceDir = EnsureDirectory(::std::string_view{i_vArgument[0uz]}, "Source directory required as first argument!");
	Directory const BinaryDir = EnsureDirectory(SourceDir / ::std::string_view{i_vArgument[1uz]}, "Relative binary directory required as second argument!");
	::std::string_view const sRootModuleName = i_vArgument[2uz];

	CombinedFileStore
		vAllFiles
	{	SourceDir
	,	BinaryDir
	};
	DirectoryRelativeStream
		cout
	{	SourceDir
	,	::std::cout
	};

	UnorderedVector<ModuleInterface>
		vModuleInterfaces
	;
	vModuleInterfaces.reserve(vAllFiles.vHeaderFiles.size());

	::std::map<::std::string, Module>
		vModules
	;

	for	(	auto const
			&	rHeader
		:	vAllFiles.vHeaderFiles
		)
	{
		auto const
			vFileName
		=	rHeader.m_vPath.filename().replace_extension()
		;

		::std::string_view
			sFileName
		=	vFileName.c_str()
		;

		if	(	sFileName.starts_with(sRootModuleName)
			)
			sFileName.remove_prefix(sRootModuleName.size());

		auto const
			vRelativePath
		=	SourceDir.RelativePath(rHeader.m_vPath).remove_filename()
		;

		ModuleInterface
			vModuleInterface
		{	.m_vInterface
			=	rHeader
		,	.m_sModuleName
		=	{	sRootModuleName.begin()
			,	sRootModuleName.end()
			}
		,	.m_sPartitionName = ""
		,	.m_bExport = true
		};


		::std::stringstream
			vDirectories
		{	vRelativePath
		};

		for	(	::std::string
					sDirectory
			;	getline
				(	vDirectories
				,	sDirectory
				,	::std::filesystem::path::preferred_separator
				)
			;
			)
		{
			if	(sDirectory == "src")
			{
				vModuleInterface.m_bExport = false;
				continue;
			}
			if	(sDirectory == "include")
				continue;

			//	trim matching submodule names from file name to create the partition name
			//	the partition name may end up empty, in which case the file is assumed to be
			//	the primary module interface
			if	(	sFileName.starts_with(sDirectory)
				)
				sFileName.remove_prefix(sDirectory.size());

			//	convert all directories (excluding include and src) to submodules
			vModuleInterface.m_sModuleName += '.';
			vModuleInterface.m_sModuleName += sDirectory;
		}

		vModuleInterface.m_sPartitionName += sFileName;
		auto& rModule = vModules[vModuleInterface.m_sModuleName];

		if	(vModuleInterface.m_sPartitionName.empty())
		{
			rModule.m_vPrimaryInterface = vModuleInterface;
		}
		else
		if	(	vModuleInterface.m_bExport
			)
		{
			rModule.m_vPartitionInterfaces.push_back(vModuleInterface.m_sPartitionName);
		}

		vModuleInterfaces.push_back(vModuleInterface);
	}

	for	(	auto
				nIndex = 0uz
		;	nIndex < vModules.size()
		;
		)
	{
		auto vIterator = vModules.begin();
		std::advance(vIterator, nIndex);
		auto& [sModuleName, vModule] = *vIterator;
		auto const sModulePrefix = sModuleName + ".";
		// merge modules
		auto const
			nErasedCount
		=	std::erase_if
			(	vModules
			,	[	&rDestinationVector = vModule.m_vPartitionInterfaces
				,	&sModulePrefix
				]	(	auto
						&	i_rPair
					)
				{
					std::string_view const sEraseModuleName = i_rPair.first;
					bool const bErase
					=	sEraseModuleName.starts_with(sModulePrefix)
					;
					if	(bErase)
					{
						auto const sTrimName = sEraseModuleName.substr(sModulePrefix.size());
						auto& rSourceVector = i_rPair.second.m_vPartitionInterfaces;

						for	(auto& rPartitionName : rSourceVector)
						{
							rPartitionName = std::format("{}.{}", sTrimName, rPartitionName);
						}
						rDestinationVector.TakeOver(rSourceVector);
					}

					return bErase;
				}
			)
		;

		for (	auto
				&	rModuleInterface
			:	vModuleInterfaces
			)
		{
			if	(rModuleInterface.m_sModuleName.starts_with(sModulePrefix))
			{
				auto const sTrimName = std::string_view{rModuleInterface.m_sModuleName}.substr(sModulePrefix.size());
				rModuleInterface.m_sPartitionName = std::format("{}.{}", sTrimName, rModuleInterface.m_sPartitionName);
				rModuleInterface.m_sModuleName.erase(sModulePrefix.size() - 1uz);
			}
		}

		if (nErasedCount == 0)
			++nIndex;
		else
			nIndex = 0; // restart loop
	}


	for	(	ModuleInterface const
			&	rModuleInterface
		:	vModuleInterfaces
		)
	{
		try
		{	ModuleInterfaceConverter
				vInterfaceConverter
			{	rModuleInterface
			};
			vInterfaceConverter.ReadModule
			(	vModuleInterfaces
			);

			if	(not rModuleInterface.m_vInterface.IsHeaderOnly())
			{
				// cannot share the same partitionname among multiple module units!
				ModuleInterfaceConverter
					vImplementationConverter
				{	rModuleInterface.m_vInterface.m_vImplementation.m_vPath
				,	rModuleInterface.m_sModuleName
				,	/*no partition name*/ ""
				,	rModuleInterface.m_vInterface.m_vImplementation.GetDependencies()
				};
				vImplementationConverter.ReadModule
				(	vModuleInterfaces
				);
				vImplementationConverter.WriteModule();
			}
			vInterfaceConverter.WriteModule();
		}
		catch
			(	...
			)
		{
			cout << "Did not convert File " << rModuleInterface.m_vInterface;
		}
	}

	for	(	auto&
			[	sModuleName
			,	vModule
			]
		:	vModules
		)
	{
		vModule.m_vPartitionInterfaces.sort();

		if	(vModule.m_vPrimaryInterface.m_vInterface.m_vPath.native().empty())
		{
			::std::string
				sFileName
			=	sModuleName
			;
			::std::replace(sFileName.begin(), sFileName.end(), '.', ::std::filesystem::path::preferred_separator);
			// use .hpp as extension for module interfaces for now
			sFileName += ".hpp";
			sFileName = sFileName.substr(sRootModuleName.size()+ 1uz);
			cout << "Creating new file " << sFileName << " as primary interface for module " << sModuleName << '\n';

			::std::filesystem::path const
				vInterfacePath
			=	SourceDir / sFileName
			;

			if	(exists(vInterfacePath))
			{
				cout << "Attempt cancelled as this file already exists!\n";
				break;
			}

			::std::ofstream
				sInterface
			{	vInterfacePath
			};
			if	(sInterface.is_open())
			{
				sInterface << "export module " << sModuleName << ';' << '\n';

				for	(	::std::string_view
							sPartition
					:	vModule.m_vPartitionInterfaces
					)
				{
					sInterface << "\nexport import :" << sPartition << ';';
				}
				sInterface << ::std::endl;
				sInterface.close();
			}
			else
			{
				cout << "Could not write to the file!\n";
			}
		}
		else
		{
			cout << "Designating existing file " << vModule.m_vPrimaryInterface.m_vInterface << " as primary module interface for module " << sModuleName << ". Please make sure the following is contained in that file:";
			for	(	::std::string_view
						sPartition
				:	vModule.m_vPartitionInterfaces
				)
			{
				cout << "\n\texport import :" << sPartition << ';';
			}
			cout << '\n';
		}
	}

	for	(	auto const
			&	rImplementation
		:	vAllFiles.vImplementationFiles
		)
	{
		try
		{
			::std::string
				sModuleName
			{	sRootModuleName.begin()
			,	sRootModuleName.end()
			};

			auto const
			vRelativePath
			=	SourceDir.RelativePath(rImplementation.m_vPath).remove_filename()
			;

			::std::stringstream
				vDirectories
			{	vRelativePath
			};

			for	(	::std::string
					sDirectory
				;	getline
					(	vDirectories
					,	sDirectory
					,	::std::filesystem::path::preferred_separator
					)
				;
				)
			{
				//	convert all directories (excluding include and src) to submodules
				if	(	sDirectory != "src"
					and	sDirectory != "include"
					)
				{
					sModuleName += '.';
					sModuleName += sDirectory;
				}
			}

			ModuleInterfaceConverter
				vConverter
			{	rImplementation.m_vPath
			,	sModuleName
			,	""
			,	rImplementation.GetDependencies()
			};
			vConverter.ReadModule
			(	vModuleInterfaces
			);
			vConverter.WriteModule();
		}
		catch
			(	...
			)
		{
			cout << "Did not convert File " << rImplementation;
		}
	}
}
