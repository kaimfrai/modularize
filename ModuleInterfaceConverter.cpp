#include "ModuleInterfaceConverter.hpp"

#include "HeaderFile.hpp"
#include "UnorderedVector.hpp"

#include <fstream>
#include <string>

namespace
{
	struct
		ModularizingCFileException
		:	::std::runtime_error
	{
		explicit(true) inline
		(	ModularizingCFileException
		)	()
		:	::std::runtime_error
			{	"Attempted to convert C File to module!"
			}
		{}
	};
}

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
			{	sPPDirective.begin() + sPPDirective.find_first_not_of(" \t\"<", sizeof("include"))
			,	sPPDirective.begin() + sPPDirective.find_last_of(">\"")
			};

			//	resolve full path first
			//	there could be multiple files with the same name in different directories
			if	(	auto const
						vPathIt
					=	m_rModuleInterface.m_vInterface.GetDependencies().find_if
						(	[	sInclude
							]	(	::std::filesystem::path const
									&	i_rDependencyPath
								)
							->	bool
							{	return
								i_rDependencyPath.native().ends_with
								(	sInclude
								);
							}
						)
				;	vPathIt
				!=	m_rModuleInterface.m_vInterface.GetDependencies().end()
				)
			{
				if	(	auto const
							vImportedModuleInterfaceIt
						=	i_rModuleInterfaces.find_if
							(	[	&rPath
									=	*vPathIt
								]	(	ModuleInterface const
										&	i_rModuleInterface
									)
								->	bool
								{	return
										i_rModuleInterface.m_vInterface.m_vPath
									==	rPath
									;
								}
							)
					;	vImportedModuleInterfaceIt
					!=	i_rModuleInterfaces.end()
					)
				{
					//	no check for dublicate imports to retain preceeding comments
					if	(	not
							vImportedModuleInterfaceIt->m_sPartitionName.empty()
						and	(	vImportedModuleInterfaceIt->m_sModuleName
							==	m_rModuleInterface.m_sModuleName
							)
						)
					{
						FlushPending(m_vPartitionComment);
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
						FlushPending(m_vImportComment);
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
			}

			//	no module interface found => global header
			FlushPending(m_vGlobalFragment) << i_vLine << '\n';
			return &m_vGlobalFragment;
		}

		//	bundle all other directives together with next fragment
		m_vPending << i_vLine << '\n';
		return &i_rCurrentFragment;
	}

	if	(	sNoWhitespace.starts_with("extern")
		)
	{
		::std::string_view const
			sExtern
		{	sNoWhitespace.begin() + sNoWhitespace.find_first_not_of(" \t", sizeof("extern"))
		,	sNoWhitespace.end()
		};
		if	(sExtern.starts_with("\"C\""))
		{
			//	extern C indicates that this file is used as a C file, do not convert those
			throw ModularizingCFileException{};
		}
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

		if	(	sNoWhitespace.starts_with("using")
			)
		{
			::std::string_view const
				sUsing
			{	sNoWhitespace.begin() + sNoWhitespace.find_first_not_of(" \t", sizeof("using"))
			,	sNoWhitespace.end()
			};

			auto const vScopePos = sNoWhitespace.find_first_of(":");
			auto const vAssignPos = sNoWhitespace.find_first_of("=");


			if	(	sUsing.starts_with("namespace")
				or	(	vScopePos
					<	vAssignPos
					)
				)
			{
				//	do not export using directives unless they are an alias
				FlushPending(m_vNamedFragment) << i_vLine << '\n';
				return &m_vNamedFragment;
			}
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
			)
		{
			//	new line for export makes git diff cleaner
			m_vNamedFragment << '\n';
			FlushPending(m_vNamedFragment) << "export\n" << i_vLine << '\n';
			return &m_vNamedFragment;
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
	=	m_rModuleInterface.m_vInterface.m_vPath
	;

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
	=	m_rModuleInterface.m_vInterface.m_vPath
	;

	if	(	not
			m_vHeadComment.view
			().	empty
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
			m_vGlobalFragment.view
			().	empty
			()
		)
	{		vWriter
		<<	"module;\n\n"
		<<	m_vGlobalFragment.str()
		//	new line between global and named fragment
		<<	'\n';

	}

	//	write named module fragment
		vWriter
	<<	"export module "
	<<	m_rModuleInterface.m_sModuleName
	;
	if	(	not
			m_rModuleInterface.m_sPartitionName.empty()
		)
		(	vWriter
		<<	':'
		<<	m_rModuleInterface.m_sPartitionName
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
			vWriter << "import :" << sPartitionName << ';' << '\n';
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
			vWriter << "import " << sImportName << ';' << '\n';
		}
	}

	if	(	m_vNamedFragment.str()[0uz]
		!=	'\n'
		)
		vWriter << '\n';
	vWriter << m_vNamedFragment.str();
}

auto
(	::Modularize::ModuleInterfaceConverter::ProcessFile
)	(	UnorderedVector<ModuleInterface> const
		&	i_rModuleInterfaces
	)
->	void
{
	ReadModule(i_rModuleInterfaces);
	WriteModule();
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

	for	(	auto const
			&	i_rHeader
		:	vAllFiles.vHeaderFiles
		)
	{
		auto const
			vFileName
		=	i_rHeader.m_vPath.filename().replace_extension()
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
		=	SourceDir.RelativePath(i_rHeader.m_vPath).remove_filename()
		;

		ModuleInterface
			vModuleInterface
		{	.m_vInterface
			=	i_rHeader
		,	.m_sModuleName
		=	{	sRootModuleName.begin()
			,	sRootModuleName.end()
			}
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
			//	trim matching submodule names from file name to create the partition name
			//	the partition name may end up empty, in which case the file is assumed to be
			//	the primary module interface
			if	(	sFileName.starts_with(sDirectory)
				)
				sFileName.remove_prefix(sDirectory.size());

			//	convert all directories (excluding include and src) to submodules
			if	(	sDirectory != "src"
				and	sDirectory != "include"
				)
			{
				vModuleInterface.m_sModuleName += '.';
				vModuleInterface.m_sModuleName += sDirectory;
			}
		}

		vModuleInterface.m_sPartitionName = sFileName;
		vModuleInterfaces.push_back(vModuleInterface);
	}

	for	(	ModuleInterface const
			&	rModuleInterface
		:	vModuleInterfaces
		)
	{
		ModuleInterfaceConverter
			vConverter
		{	rModuleInterface
		};

		try
		{
			vConverter.ProcessFile
			(	vModuleInterfaces
			);
		}
		catch
			(	ModularizingCFileException const
				&
			)
		{
			cout << "Did not convert C-File " << rModuleInterface.m_vInterface;
		}
	}

	for	(	auto const
			&	rImplementation
		:	vAllFiles.vImplementationFiles
		)
	{
		cout << "Did not convert implementation file without interface " << rImplementation;
	}
}
