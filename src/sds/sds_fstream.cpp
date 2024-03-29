// Copyright (c) 2018-present Art of the State LLC
// Under MIT License. See LICENSE.md for details

#include "sds/sds_fstream.h"

#include <stdio.h>

#include <limits>
#include <cstddef>

#ifdef WIN32
#	include <io.h>
#else
#	include <unistd.h>
#endif

#ifdef __APPLE__
#	include <sys/fcntl.h>
#endif

namespace sds
{
	fstream::fstream() : m_handle( 0 ), m_statusBits( 0 ), m_canRead( false ), m_canWrite( false ) {}
	//-------------------------------------------------------------------------
	fstream::fstream( const char *fullpath, FileOpenMode mode, const bool bFromBundle ) :
		m_handle( 0 ),
		m_statusBits( 0 ),
		m_canRead( false ),
		m_canWrite( false )
	{
		open( fullpath, mode, bFromBundle );
	}
	//-------------------------------------------------------------------------
	fstream::fstream( const std::string &fullpath, FileOpenMode mode, const bool bFromBundle ) :
		m_handle( 0 ),
		m_statusBits( 0 ),
		m_canRead( false ),
		m_canWrite( false )
	{
		open( fullpath.c_str(), mode, bFromBundle );
	}
	//-------------------------------------------------------------------------
	fstream::~fstream() { close(); }
	//-------------------------------------------------------------------------
	void fstream::open( const std::string &fullpath, FileOpenMode mode, const bool bFromBundle )
	{
		open( fullpath.c_str(), mode, bFromBundle );
	}
	//-------------------------------------------------------------------------
	void fstream::open( const char *fullpath, FileOpenMode mode, const bool )
	{
		close();

		const char *modeStr = 0;

		switch( mode )
		{
		case fstream::Input:
		case fstream::InputEnd:
			m_canRead = true;
			modeStr = "rb";
			break;
		case fstream::OutputKeep:
		case fstream::OutputKeepEnd:
			m_canWrite = true;
			modeStr = "r+b";
			break;
		case fstream::OutputDiscard:
			m_canWrite = true;
			modeStr = "w+b";
			break;
		case fstream::InOutKeep:
			m_canRead = true;
			m_canWrite = true;
			modeStr = "r+b";
			break;
		case fstream::InOutEnd:
			m_canRead = true;
			m_canWrite = true;
			modeStr = "r+b";
			break;
		}

		m_handle = fopen( fullpath, modeStr );

		if( m_handle && ( mode == fstream::InputEnd || mode == fstream::InOutEnd ||
						  mode == fstream::OutputKeepEnd ) )
		{
			seek( 0, fstream::end );
		}
	}
	//-------------------------------------------------------------------------
	void fstream::close()
	{
		if( m_handle )
		{
			fclose( m_handle );
			m_handle = 0;
		}

		m_statusBits = 0u;
		m_canRead = false;
		m_canWrite = false;
	}
	//-------------------------------------------------------------------------
	bool fstream::is_open() const { return m_handle != 0; }
	//-------------------------------------------------------------------------
	bool fstream::good()
	{
		if( m_handle == 0 )
			return false;

		if( ( m_statusBits & ( fstream::badbit | fstream::failbit ) ) == 0 )
		{
			if( ferror( m_handle ) != 0 )
				m_statusBits |= fstream::badbit;
		}

		return m_statusBits == 0u || m_statusBits == fstream::eof;
	}
	//-------------------------------------------------------------------------
	bool fstream::is_eof() const { return ( m_statusBits & fstream::eof ) != 0u; }
	//-------------------------------------------------------------------------
	size_t fstream::read( char *outData, size_t sizeBytes )
	{
		if( m_canRead )
		{
			const size_t retVal = fread( outData, 1u, sizeBytes, m_handle );

			if( retVal != sizeBytes )
			{
				if( ferror( m_handle ) != 0 )
					m_statusBits |= fstream::badbit;
				if( feof( m_handle ) != 0 )
					m_statusBits |= fstream::eof;
			}

			return retVal;
		}
		else
		{
			m_statusBits |= fstream::failbit;
			return std::numeric_limits<size_t>::max();
		}
	}
	//-------------------------------------------------------------------------
	size_t fstream::write( const char *inData, size_t sizeBytes )
	{
		if( m_canWrite )
		{
			const size_t retVal = fwrite( inData, 1u, sizeBytes, m_handle );

			if( retVal != sizeBytes )
				m_statusBits |= fstream::badbit;

			return retVal;
		}
		else
		{
			m_statusBits |= fstream::failbit;
			return std::numeric_limits<size_t>::max();
		}
	}
	//-------------------------------------------------------------------------
	void fstream::seek( ptrdiff_t dir, Whence whence )
	{
		if( !good() )
		{
			m_statusBits |= fstream::failbit;
			return;
		}

		// If we're at EOF and we can't absolutely be sure we're no longer EOF,
		// then we need the file size to know if we can clear the EOF flag
		size_t fileSize = 0u;
		if( is_eof() )
		{
			fileSize = tell();  // We're at EOF, so tell() returns file size
			if( !good() )
				return;
		}

		int fileWhence = 0;
		switch( whence )
		{
		case fstream::beg:
			fileWhence = SEEK_SET;
			break;
		case fstream::cur:
			fileWhence = SEEK_CUR;
			break;
		case fstream::end:
			fileWhence = SEEK_END;
			break;
		}

		const int status = fseek( m_handle, dir, fileWhence );

		if( status != 0 )
		{
			m_statusBits |= fstream::failbit;

			if( ferror( m_handle ) != 0 )
				m_statusBits |= fstream::badbit;
		}
		else
		{
			// Clear eof bit but only if there are no errors
			if( m_statusBits == StatusBits::eof )
			{
				const size_t currPos = tell();
				if( currPos < fileSize )
					m_statusBits = 0u;
			}
		}
	}
	//-------------------------------------------------------------------------
	size_t fstream::tell()
	{
		if( !good() )
		{
			m_statusBits |= fstream::failbit;
			return std::numeric_limits<size_t>::max();
		}

		const long int position = ftell( m_handle );
		if( position == -1L )
			m_statusBits |= fstream::badbit;
		return static_cast<size_t>( position );
	}
	//-------------------------------------------------------------------------
	size_t fstream::getFileSize( const bool bRestoreOffset )
	{
		if( !good() )
		{
			m_statusBits |= fstream::failbit;
			return std::numeric_limits<size_t>::max();
		}

		size_t currentPos = 0u;
		if( bRestoreOffset )
			currentPos = tell();
		seek( 0, fstream::end );
		const size_t fileSize = tell();
		if( bRestoreOffset )
			seek( static_cast<ptrdiff_t>( currentPos ), fstream::beg );
		return fileSize;
	}
	//-------------------------------------------------------------------------
	int fstream::flush()
	{
		if( m_canWrite )
		{
			int status = fflush( m_handle );
			if( status != 0 )
				m_statusBits |= fstream::badbit;

			return status;
		}
		else
		{
			m_statusBits |= fstream::failbit;
			return -1;
		}
	}
	//-------------------------------------------------------------------------
	void fstream::fsync( bool preferDataSync )
	{
		if( !m_canWrite )
		{
			m_statusBits |= fstream::failbit;
			return;
		}

		flush();

		int fileDescriptor = fileno( m_handle );

		if( fileDescriptor == -1 )
		{
			m_statusBits |= fstream::badbit;
			return;
		}

		int status = 0;
#ifndef _WIN32
#	ifdef __APPLE__
		status = fcntl( fileDescriptor, F_FULLFSYNC, NULL );
		if( status )
		{
			// If we are not on a file system that supports this,
			// then fall back to a plain fsync.
			status = ::fsync( fileDescriptor );
			if( status != 0 )
				m_statusBits |= fstream::badbit;
		}
#	else
		if( !preferDataSync )
			status = ::fsync( fileDescriptor );
		else
			status = ::fdatasync( fileDescriptor );

		if( status != 0 )
			m_statusBits |= fstream::badbit;
#	endif
#else
		status = _commit( fileDescriptor );
		if( status != 0 )
			m_statusBits |= fstream::badbit;
#endif
	}
	//-------------------------------------------------------------------------
	template <>
	size_t fstream::read<bool>( bool &outValue )
	{
		uint8_t value = 0;
		const size_t retVal = read<uint8_t>( value );
		outValue = value != 0;
		return retVal;
	}
	//-------------------------------------------------------------------------
	template <>
	size_t fstream::write<bool>( bool inValue )
	{
		uint8_t value = inValue ? 1u : 0u;
		return write<uint8_t>( value );
	}
}  // namespace sds
