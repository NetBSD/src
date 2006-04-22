/* -*-C++-*-	$NetBSD: file_manager.cpp,v 1.6.6.1 2006/04/22 11:37:28 simonb Exp $	*/

/*-
 * Copyright(c) 1996, 2001, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthias Drochner. and UCHIYAMA Yasushi.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <console.h>
#include <file.h>
#include <limits.h>

__BEGIN_DECLS
#include <string.h>
#include <zlib.h>
__END_DECLS

static struct z_stream_s __stream;	// XXX for namespace.

void
FileManager::_reset()
{
	_stream = &__stream;
	memset(_stream, 0, sizeof(struct z_stream_s));
	_z_err = 0;
	_z_eof = 0;
	_crc = 0;
	_compressed = 0;
}

FileManager::~FileManager()
{
	delete _file;
}

BOOL
FileManager::setRoot(TCHAR *drive)
{
	return _file->setRoot(drive);
}

BOOL
FileManager::open(const TCHAR *name, uint32_t flags)
{
	if (!_file->open(name, flags))
		return FALSE;

	_reset();

	if (inflateInit2(_stream, -15) != Z_OK)
		goto errout;
	_stream->next_in = _inbuf;

	_check_header(); // skip the .gz header

	return TRUE;
 errout:
	_file->close();
	return FALSE;
}

size_t
FileManager::read(void *buf, size_t len, off_t ofs)
{
	if (ofs != -1)
		seek(ofs);

	return _read(buf, len);
}

size_t
FileManager::_read(void *buf, size_t len)
{
	// starting point for crc computation
	uint8_t *start = reinterpret_cast<uint8_t *>(buf);

	if (_z_err == Z_DATA_ERROR || _z_err == Z_ERRNO) {
		return -1;
	}
	if (_z_err == Z_STREAM_END) {
		return 0;  // EOF
	}
	_stream->next_out = reinterpret_cast<uint8_t *>(buf);
	_stream->avail_out = len;

	int got;
	while (_stream->avail_out != 0) {
		if (!_compressed) {
			// Copy first the lookahead bytes
			uint32_t n = _stream->avail_in;
			if (n > _stream->avail_out)
				n = _stream->avail_out;
			if (n > 0) {
				memcpy(_stream->next_out, _stream->next_in, n);
				_stream->next_out  += n;
				_stream->next_in   += n;
				_stream->avail_out -= n;
				_stream->avail_in  -= n;
			}
			if (_stream->avail_out > 0) {
				got = _file->read(_stream->next_out,
				    _stream->avail_out);
				if (got == -1) {
					return(got);
				}
				_stream->avail_out -= got;
			}
			return(int)(len - _stream->avail_out);
		}

		if (_stream->avail_in == 0 && !_z_eof) {
			got = _file->read(_inbuf, Z_BUFSIZE);
			if (got <= 0)
				_z_eof = 1;

			_stream->avail_in = got;
			_stream->next_in = _inbuf;
		}

		_z_err = inflate(_stream, Z_NO_FLUSH);

		if (_z_err == Z_STREAM_END) {
			/* Check CRC and original size */
			_crc = crc32(_crc, start,(unsigned int)
			    (_stream->next_out - start));
			start = _stream->next_out;

			if (_get_long() != _crc ||
			    _get_long() != _stream->total_out) {
				_z_err = Z_DATA_ERROR;
			} else {
				/* Check for concatenated .gz files: */
				_check_header();
				if (_z_err == Z_OK) {
					inflateReset(_stream);
					_crc = crc32(0L, Z_NULL, 0);
				}
			}
		}
		if (_z_err != Z_OK || _z_eof)
			break;
	}

	_crc = crc32(_crc, start,(unsigned int)(_stream->next_out - start));

	return(int)(len - _stream->avail_out);
}

size_t
FileManager::write(const void *buf, size_t bytes, off_t ofs)
{
	return _file->write(buf, bytes, ofs);
}

size_t
FileManager::size()
{
	return _file->size();
}

BOOL
FileManager::close()
{
	inflateEnd(_stream);

	return _file->close();
}

size_t
FileManager::_skip_compressed(off_t toskip)
{
#define	DUMMYBUFSIZE 256
	char dummybuf[DUMMYBUFSIZE];

	size_t skipped = 0;

	while (toskip > 0) {
		size_t toread = toskip;
		if (toread > DUMMYBUFSIZE)
			toread = DUMMYBUFSIZE;

		size_t nread = _read(dummybuf, toread);
		if ((int)nread < 0)
			return nread;

		toskip  -= nread;
		skipped += nread;

		if (nread != toread)
			break;
	}

	return skipped;
}

size_t
FileManager::realsize()
{
	if (!_compressed)
		return size();

	off_t pos = _stream->total_out;
	size_t sz = _skip_compressed(INT_MAX);
	seek(pos);

	return sz;
}

BOOL
FileManager::seek(off_t offset)
{

	if (!_compressed) {
		_file->seek(offset);
		_stream->avail_in = 0;

		return TRUE;
	}
	/* if seek backwards, simply start from the beginning */
	if (offset < _stream->total_out) {
		_file->seek(0);

		inflateEnd(_stream);
		_reset(); /* this resets total_out to 0! */
		inflateInit2(_stream, -15);
		_stream->next_in = _inbuf;

		_check_header(); /* skip the .gz header */
	}

	/* to seek forwards, throw away data */
	if (offset > _stream->total_out) {
		off_t toskip = offset - _stream->total_out;
		size_t skipped = _skip_compressed(toskip);

		if (skipped != toskip)
			return FALSE;
	}

	return TRUE;
}

//
// GZIP util.
//
int
FileManager::_get_byte()
{

	if (_z_eof)
		return(EOF);

	if (_stream->avail_in == 0) {
		int got;

		got = _file->read(_inbuf, Z_BUFSIZE);
		if (got <= 0) {
			_z_eof = 1;
			return EOF;
		}
		_stream->avail_in = got;
		_stream->next_in = _inbuf;
	}
	_stream->avail_in--;
	return *(_stream->next_in)++;
}

uint32_t
FileManager::_get_long()
{
	uint32_t x = static_cast<uint32_t>(_get_byte());
	int c;

	x +=(static_cast<uint32_t>(_get_byte())) << 8;
	x +=(static_cast<uint32_t>(_get_byte())) << 16;
	c = _get_byte();
	if (c == EOF)
		_z_err = Z_DATA_ERROR;
	x +=(static_cast<uint32_t>(c)) << 24;

	return x;
}

void
FileManager::_check_header()
{
	int method; /* method byte */
	int flags;  /* flags byte */
	unsigned int len;
	int c;

	/* Check the gzip magic header */
	for (len = 0; len < 2; len++) {
		c = _get_byte();
		if (c == _gz_magic[len])
			continue;
		if ((c == EOF) &&(len == 0))  {
			/*
			 * We must not change _compressed if we are at EOF;
			 * we may have come to the end of a gzipped file and be
			 * check to see if another gzipped file is concatenated
			 * to this one. If one isn't, we still need to be able
			 * to lseek on this file as a compressed file.
			 */
			return;
		}
		_compressed = 0;
		if (c != EOF) {
			_stream->avail_in++;
			_stream->next_in--;
		}
		_z_err = _stream->avail_in != 0 ? Z_OK : Z_STREAM_END;
		return;
	}
	_compressed = 1;
	method = _get_byte();
	flags = _get_byte();
	if (method != Z_DEFLATED ||(flags & RESERVED) != 0) {
		_z_err = Z_DATA_ERROR;
		return;
	}

	/* Discard time, xflags and OS code: */
	for (len = 0; len < 6; len++)
		(void)_get_byte();

	if ((flags & EXTRA_FIELD) != 0) {
		/* skip the extra field */
		len  = (unsigned int)_get_byte();
		len +=((unsigned int)_get_byte()) << 8;
		/* len is garbage if EOF but the loop below will quit anyway */
		while (len-- != 0 && _get_byte() != EOF) /*void*/;
	}
	if ((flags & ORIG_NAME) != 0) {
		/* skip the original file name */
		while ((c = _get_byte()) != 0 && c != EOF) /*void*/;
	}
	if ((flags & COMMENT) != 0) {
		/* skip the .gz file comment */
		while ((c = _get_byte()) != 0 && c != EOF) /*void*/;
	}
	if ((flags & HEAD_CRC) != 0) {  /* skip the header crc */
		for (len = 0; len < 2; len++)
			(void)_get_byte();
	}
	_z_err = _z_eof ? Z_DATA_ERROR : Z_OK;
}
