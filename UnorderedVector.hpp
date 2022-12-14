#pragma once

#include <algorithm>
#include <vector>
#include <utility>

namespace
	Modularize
{
	template
		<	typename
				t_tElement
		>
	class
		UnorderedVector
	{
		using VectorType = ::std::vector<t_tElement>;
		VectorType
			m_vElements
		;

	public:

		using size_type = typename VectorType::size_type;
		using value_type = typename VectorType::value_type;
		using reference = typename VectorType::reference;
		using const_reference = typename VectorType::const_reference;
		using iterator = typename VectorType::iterator;
		using const_iterator = typename VectorType::const_iterator;

		[[nodiscard]]
		auto
		(	empty
		)	()	const
		->	bool
		{	return m_vElements.empty();	}

		[[nodiscard]]
		auto
		(	size
		)	()	const
		->	size_type
		{	return m_vElements.size();	}

		[[nodiscard]]
		auto
		(	front
		)	()	&
		->	reference
		{	return m_vElements.front();	}

		[[nodiscard]]
		auto
		(	operator[]
		)	(	size_type
					i_nIndex
			)	&
		->	reference
		{	return m_vElements[i_nIndex];	}

		[[nodiscard]]
		auto
		(	operator[]
		)	(	size_type
					i_nIndex
			)	const&
		->	const_reference
		{	return m_vElements[i_nIndex];	}

		[[nodiscard]]
		auto
		(	front
		)	()	const&
		->	const_reference
		{	return m_vElements.front();	}

		[[nodiscard]]
		auto
		(	back
		)	()	&
		->	reference
		{	return m_vElements.back();	}
		[[nodiscard]]

		auto
		(	back
		)	()	const&
		->	const_reference
		{	return m_vElements.back();	}

		auto
		(	reserve
		)	(	size_type
					i_nCapacity
			)	&
		->	void
		{	return
			m_vElements.reserve
			(	i_nCapacity
			);
		}

		auto
		(	push_back
		)	(	value_type const
				&	i_rValue
			)	&
		->	void
		{	return
			m_vElements.push_back
			(	i_rValue
			);
		}

		template
			<	typename
				...	t_tpArgument
			>
		auto
		(	emplace_back
		)	(	t_tpArgument&&
				...	i_rpArgument
			)
		->	reference
		{	return
			m_vElements.emplace_back
			(	::std::forward<t_tpArgument>
				(	i_rpArgument
				)
				...
			);
		}

		[[nodiscard]]
		auto
		(	begin
		)	()	&
		->	iterator
		{	return m_vElements.begin();	}

		[[nodiscard]]
		auto
		(	begin
		)	()	const&
		->	const_iterator
		{	return m_vElements.begin();	}

		[[nodiscard]]
		auto
		(	end
		)	()	&
		->	iterator
		{	return m_vElements.end();	}

		[[nodiscard]]
		auto
		(	end
		)	()	const&
		->	const_iterator
		{	return m_vElements.end();	}

		auto
		(	pop_back
		)	()	&
		->	void
		{	return m_vElements.pop_back();	}

		[[nodiscard]]
		auto
		(	find
		)	(	value_type const
				&	i_rValue
			)	&
		->	iterator
		{	return
			::std::find
			(	begin()
			,	end()
			,	i_rValue
			);
		}

		[[nodiscard]]
		auto
		(	find
		)	(	value_type const
				&	i_rValue
			)	const&
		->	const_iterator
		{	return
			::std::find
			(	begin()
			,	end()
			,	i_rValue
			);
		}

		[[nodiscard]]
		auto
		(	find_if
		)	(	auto const
				&	i_rPredicate
			)	&
		->	iterator
		{	return
			::std::find_if
			(	begin()
			,	end()
			,	i_rPredicate
			);
		}

		[[nodiscard]]
		auto
		(	find_if
		)	(	auto const
				&	i_rPredicate
			)	const&
		->	const_iterator
		{	return
			::std::find_if
			(	begin()
			,	end()
			,	i_rPredicate
			);
		}

		auto
		(	erase_if
		)	(	auto const
				&	i_rPredicate
			)	&
		->	std::size_t
		{	return
			::std::erase_if
			(	m_vElements
			,	i_rPredicate
			);
		}

		[[nodiscard]]
		auto
		(	contains
		)	(	value_type const
				&	i_rValue
			)	const&
		->	bool
		{	return
			(	end()
			!=	find(i_rValue)
			);
		}

		[[nodiscard]]
		auto
		(	none_of
		)	(	auto const
				&	i_rPredicate
			)	const&
		->	bool
		{	return
			::std::none_of
			(	begin()
			,	end()
			,	i_rPredicate
			);
		}

		[[nodiscard]]
		auto
		(	any_of
		)	(	auto const
				&	i_rPredicate
			)	const&
		->	bool
		{	return
			::std::any_of
			(	begin()
			,	end()
			,	i_rPredicate
			);
		}

		auto
		(	sort
		)	()
		->	void
		{	return
			::std::sort
			(	begin()
			,	end()
			);
		}

		auto
		(	TakeOver
		)	(	UnorderedVector
				&	i_rSource
			)
		{
			m_vElements.reserve(size() + i_rSource.size());

			m_vElements.insert
			(	m_vElements.end()
			,	std::make_move_iterator(i_rSource.begin())
			,	std::make_move_iterator(i_rSource.end())
			);
			i_rSource.m_vElements.clear();
		}

		auto
		(	SwapOut
		)	(	iterator
					i_vPosition
			)	&
		->	value_type
		{
			if	(i_vPosition == end())
				return {};

			using ::std::swap;
			swap(*i_vPosition, back());
			auto const vResult = ::std::move(back());
			pop_back();
			return vResult;
		}

		auto
		(	SwapOut
		)	(	value_type const
				&	i_rValue
			)	&
		->	value_type
		{	return
			SwapOut
			(	find
				(	i_rValue
				)
			);
		}

		auto
		(	SwapOut
		)	(	size_type
					i_nIndex
			)	&
		->	value_type
		{	return
			SwapOut
			(	begin()
			+	static_cast<::std::ptrdiff_t>(i_nIndex)
			);
		}

		template
			<	typename
				...	t_tpPredicate
			>
		auto
		(	FindByPriority
		)	(	t_tpPredicate const
				&
				...	i_rpPredicate
			)	&
		->	iterator
		{
			auto
				vPosition
			=	end()
			;

			static_cast<void>
			((	...
			or	(	(	vPosition
					=	find_if
						(	i_rpPredicate
						)
					)
				!=	end()
				)
			));
			return vPosition;
		}

		template
			<	typename
				...	t_tpPredicate
			>
		[[nodiscard]]
		auto
		(	FindByPriority
		)	(	t_tpPredicate const
				&
				...	i_rpPredicate
			)	const&
		->	const_iterator
		{
			const_iterator
				vPosition
			=	end()
			;

			static_cast<void>
			((	...
			or	(	(	vPosition
					=	find_if
						(	i_rpPredicate
						)
					)
				!=	end()
				)
			));
			return vPosition;
		}
	};
}
