#include <assert.h>
#include <array>
#include <cstddef>
#include <iterator>

namespace sds
{
	/// sds::static_vector acts similar to std::vector, but max capacity is defined at compile time
	/// and uses an std::array under the hood so that data can live in the stack if possible.
	template <typename T, std::size_t N>
	class static_vector
	{
	public:
		using iterator = typename std::array<T, N>::iterator;
		using const_iterator = typename std::array<T, N>::const_iterator;

		static_vector() : m_size( 0 ) {}

		void push_back( const T &value )
		{
			if( m_size < N )
				m_data[m_size++] = value;
		}

		void pop_back()
		{
			if( m_size > 0u )
				--m_size;
		}

		iterator erase( iterator toErase )
		{
			if( toErase == end() )
				return toErase;

			assert( m_size > 0u && toErase >= begin() && toErase <= end() );
			iterator itor = toErase;
			iterator endt = this->end();
			while( itor + 1u != endt )
			{
				*itor = *( itor + 1u );
				++itor;
			}
			--m_size;
			return toErase;
		}

		iterator       begin() { return m_data.begin(); }
		iterator       end() { return m_data.begin() + m_size; }
		const_iterator begin() const { return m_data.begin(); }
		const_iterator end() const { return m_data.begin() + m_size; }

		std::size_t size() const { return m_size; }
		std::size_t capacity() const { return N; }
		bool        empty() const { return m_size == 0; }
		T          &operator[]( std::size_t i ) { return m_data[i]; }
		const T    &operator[]( std::size_t i ) const { return m_data[i]; }

	private:
		std::array<T, N> m_data;
		std::size_t      m_size;
	};
}  // namespace sds
