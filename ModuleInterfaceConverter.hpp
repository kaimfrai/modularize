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
		ModuleInterface const
		&	m_rModuleInterface
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

	public:
		explicit(true)
		(	ModuleInterfaceConverter
		)	(	ModuleInterface const
				&	i_rModuleInterface
			)
		:	m_rModuleInterface
			{	i_rModuleInterface
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
		,	m_vNamedFragment
			{	""
			}
		,	m_nBlockCommentCounter
			{}
		,	m_nNestedPPDirectiveCounter
			{}
		,	m_nNestedScopeCounter
			{}
		{}

		auto
		(	ProcessFile
		)	(	UnorderedVector<ModuleInterface> const
				&	i_rModuleInterfaces
			)
		->	void
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
