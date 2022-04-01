#include "ModuleInterfaceConverter.hpp"

#include "HeaderFile.hpp"
#include "UnorderedVector.hpp"

#include <fstream>
#include <string>

auto
(	::Modularize::ModuleInterfaceConverter::FlushPending
)	(	::std::stringstream
		&	i_rTargetFragment
	)
->	::std::stringstream&
{
	i_rTargetFragment << m_vPending.str();
	m_vPending.clear();
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
	::std::string_view const
		sNoWhitespace
	{	i_vLine.begin() + i_vLine.find_first_not_of(" \t")
	,	i_vLine.end()
	};
	//	full-line comments go to the next fragment
	if	(	sNoWhitespace.starts_with("//")
		)
	{
		((&i_rCurrentFragment == &m_vHeadComment) ? m_vHeadComment : m_vPending) << i_vLine << '\n';
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
		((&i_rCurrentFragment == &m_vHeadComment) ? m_vHeadComment : m_vPending) << i_vLine << '\n';
		return &i_rCurrentFragment;
	}


	//	skip new lines ouside of scopes
	if	(	sNoWhitespace.empty()
		and	m_nNestedScopeCounter > 0uz
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
			if	(	not
					sPragma.starts_with("once")
				)
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
					FlushPending(m_vNamedFragment) << "import ";
					if	(	not
							vImportedModuleInterfaceIt->m_sPartitionName.empty()
						and	(	vImportedModuleInterfaceIt->m_sModuleName
							==	m_rModuleInterface.m_sModuleName
							)
						)
					{
						m_vNamedFragment << ':' << vImportedModuleInterfaceIt->m_sPartitionName;
					}
					else
						m_vNamedFragment << vImportedModuleInterfaceIt->m_sModuleName;

					m_vNamedFragment << ';' << '\n';
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

	//	global entity needs to be exported
	if	(	nOldNestedScopeCounter == 0uz
		and	(	sNoWhitespace.starts_with("namespace")
			or	sNoWhitespace.starts_with("typedef")
			or	sNoWhitespace.starts_with("using")
			or	sNoWhitespace.starts_with("class")
			or	sNoWhitespace.starts_with("struct")
			or	sNoWhitespace.starts_with("union")
			)
		)
	{
		//	prepend export to namespace
		FlushPending(m_vNamedFragment) << "\nexport " << i_vLine << '\n';
		return &m_vNamedFragment;
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
		//	new line between head comment and first fragment
		<<	'\n'
		;
	}

	if	(	not
			m_vGlobalFragment.view
			().	empty
			()
		)
	{		vWriter
		<<	"module;\n"
		<<	m_vGlobalFragment.str()
		//	new line between global and named fragment
		<<	'\n';

	}

	//	write named module fragment
		vWriter
	<<	"export module "
	<<	m_rModuleInterface.m_sModuleName
	;
	if	(	not	m_rModuleInterface.m_sPartitionName.empty())
			vWriter
		<<	':'
		<<	m_rModuleInterface.m_sPartitionName
		;
		vWriter
	<<	'\n' << '\n'
	;

	vWriter << m_vNamedFragment.str();
	//	new line at the end of the file
	vWriter << '\n';
}

auto
(	::Modularize::ModuleInterfaceConverter::ProcessFile
)	(	UnorderedVector<ModuleInterface> const
		&	i_rModuleInterfaces
	)
->	void
{
// 	if	(	auto const
// 				vModuleInterfaceIt
// 			=	i_rProcessed.find_if
// 				(	[	this
// 					]	(	ModuleInterface const
// 							&	i_rModuleInterface
// 						)
// 					{
// 						return i_rModuleInterface == m_rModuleInterface;
// 					}
// 				)
// 		;	vModuleInterfaceIt
// 		!=	i_rProcessed.end()
// 		)
// 		return *vModuleInterfaceIt;

	ReadModule(i_rModuleInterfaces);
	WriteModule();

// 	return i_rProcessed.emplace_back(m_rModuleInterface);
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

		vConverter.ProcessFile
		(	vModuleInterfaces
		);

		::std::cout << rModuleInterface.m_vInterface.m_vPath;
		break;
	}
}