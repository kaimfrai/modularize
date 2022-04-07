#pragma once

#include "CombinedFileStore.hpp"

#include <sstream>

namespace
	Modularize
{
	struct
		ModuleInterface
	{
		HeaderFile
			m_vInterface
		;

		::std::string
			m_sModuleName
		;
		::std::string
			m_sPartitionName
		;

		bool
			m_bExport
		;

		friend auto inline
		(	operator ==
		)	(	ModuleInterface const
				&	i_rLeft
			,	ModuleInterface const
				&	i_rRight
			)
		->	bool
		{
			return
				i_rLeft.m_vInterface
			==	i_rRight.m_vInterface
			;
		}
	};

	class
		ModuleInterfaceConverter
	{
		::std::filesystem::path const
		&	m_rFilePath
		;
		::std::string_view
			m_sModuleName
		;
		::std::string_view
			m_sPartitionName
		;

		UnorderedVector<::std::filesystem::path> const
		&	m_rDependencies
		;

		::std::stringstream
			m_vHeadComment
		;
		::std::string
			m_sHeaderGuard
		;

		::std::stringstream
			m_vPending
		;

		::std::stringstream
			m_vGlobalFragment
		;

		::std::stringstream
			m_vPartitionComment
		;

		::std::vector<::std::string_view>
			m_vPartitionImports
		;

		::std::stringstream
			m_vImportComment
		;

		::std::vector<::std::string_view>
			m_vPureImports
		;

		::std::stringstream
			m_vStandardComment
		;

		::std::vector<::std::string_view>
			m_vStandardImports
		;

		::std::stringstream
			m_vNamedFragment
		;

		::std::size_t
			m_nBlockCommentCounter
		;
		::std::size_t
			m_nNestedPPDirectiveCounter
		;
		::std::size_t
			m_nNestedScopeCounter
		;

		::std::string_view
			m_sExportModule
		;

		::std::string_view
			m_sExportImport
		;

		::std::string_view
			m_sExportEntity
		;

		auto
		(	FlushPending
		)	(	::std::stringstream
				&	i_rTargetFragment
			)
		->	::std::stringstream&
		;

		auto
		(	ParseLine
		)	(	UnorderedVector<ModuleInterface> const
				&	i_rModuleInterfaces
			,	::std::string_view
					i_vLine
			,	::std::stringstream
				&	i_rCurrentFragment
			)
		->	::std::stringstream*
		;

	public:
		explicit(true)
		(	ModuleInterfaceConverter
		)	(	::std::filesystem::path const
				&	i_rFilePath
			,	::std::string_view
					i_sModuleName
			,	::std::string_view
					i_sPartitionName
			,	UnorderedVector<::std::filesystem::path> const
				&	i_rDependencies
			)
		:	m_rFilePath
			{	i_rFilePath
			}
		,	m_sModuleName
			{	i_sModuleName
			}
		,	m_sPartitionName
			{	i_sPartitionName
			}
		,	m_rDependencies
			{	i_rDependencies
			}
		,	m_vHeadComment
			{	""
			}
		,	m_sHeaderGuard
			{	""
			}
		,	m_vPending
			{	""
			}
		,	m_vGlobalFragment
			{	""
			}
		,	m_vPartitionComment
			{	""
			}
		,	m_vPartitionImports
			{
			}
		,	m_vImportComment
			{	""
			}
		,	m_vPureImports
			{
			}
		,	m_vStandardComment
			{	""
			}
		,	m_vStandardImports
			{
			}
		,	m_vNamedFragment
			{	""
			}
		,	m_nBlockCommentCounter
			{}
		,	m_nNestedPPDirectiveCounter
			{}
		,	m_nNestedScopeCounter
			{}
		,	m_sExportModule
			{	"module "
			}
		,	m_sExportImport
			{	"import "
			}
		,	m_sExportEntity
			{	""
			}
		{}

		explicit(true)
		(	ModuleInterfaceConverter
		)	(	ModuleInterface const
				&	i_rModuleInterface
			)
		:	ModuleInterfaceConverter
			{	i_rModuleInterface.m_vInterface.m_vPath
			,	i_rModuleInterface.m_sModuleName
			,	i_rModuleInterface.m_sPartitionName
			,	i_rModuleInterface.m_vInterface.GetDependencies()
			}
		{
			if	(	i_rModuleInterface.m_bExport
				)
			{
				m_sExportModule = "export module ";
				m_sExportImport = "export import";
				//	new line for export makes git diff cleaner
				m_sExportEntity = "export\n";
			}
		}

		auto
		(	ReadModule
		)	(	UnorderedVector<ModuleInterface> const
				&	i_rModuleInterfaces
			)
		->	void
		;

		auto
		(	WriteModule
		)	()
		->	void
		;
	};

	struct
		Module
	{
		ModuleInterface
			m_vPrimaryInterface
		;
		UnorderedVector<::std::string>
			m_vPartitionInterfaces
		;
	};

	auto
	(	Modularize
	)	(	::std::span
			<	char const*
			>	i_vArgument
		)
	->	void
	;
}
