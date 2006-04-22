/* -*-C++-*-	$NetBSD: file.h,v 1.4.6.1 2006/04/22 11:37:27 simonb Exp $	*/

/*-
 * Copyright (c) 1996, 2001 The NetBSD Foundation, Inc.
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
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _HPCBOOT_FILE_H_
#define	_HPCBOOT_FILE_H_

#include <hpcboot.h>

class Console;
struct z_stream_s;

class File {
protected:
	Console *&_cons;
	BOOL _debug;
	BOOL _to_ascii(char *, const TCHAR *, size_t);
	char _ascii_filename[MAX_PATH];

public:
	File(Console *&cons) : _cons(cons) { /* NO-OP */ }
	virtual ~File() { /* NO-OP */ }
	BOOL &setDebug(void) { return _debug; }

	virtual BOOL setRoot(TCHAR *) = 0;
	virtual BOOL open(const TCHAR *, uint32_t = OPEN_EXISTING) = 0;
	virtual size_t size(void) = 0;
	virtual size_t realsize(void) { return size(); };
	virtual BOOL close(void) = 0;
	virtual size_t read(void *, size_t, off_t = -1) = 0;
	virtual size_t write(const void *, size_t, off_t = -1) = 0;
	virtual BOOL seek(off_t) = 0;
};

class FileManager : public File {
	// GZIP staff
#define	Z_BUFSIZE	1024
private:
	enum flags {
		ASCII_FLAG  = 0x01, /* bit 0 set: file probably ascii text */
		HEAD_CRC    = 0x02, /* bit 1 set: header CRC present */
		EXTRA_FIELD = 0x04, /* bit 2 set: extra field present */
		ORIG_NAME   = 0x08, /* bit 3 set: original file name present */
		COMMENT	    = 0x10, /* bit 4 set: file comment present */
		RESERVED    = 0xE0  /* bits 5..7: reserved */
	};

	struct z_stream_s *_stream;
	int _gz_magic[2];
	int _z_err;		/* error code for last stream operation */
	int _z_eof;		/* set if end of input file */
	uint8_t _inbuf[Z_BUFSIZE];	/* input buffer */
	uint32_t _crc;		/* crc32 of uncompressed data */
	BOOL _compressed;

	void _reset(void);
	int _get_byte(void);
	uint32_t _get_long(void);
	void _check_header(void);
	size_t _skip_compressed(off_t);

	off_t _cur_ofs;

private:
	File *_file;

public:
	FileManager(Console *&, enum FileOps);
	virtual ~FileManager(void);

	BOOL setRoot(TCHAR *);
	BOOL open(const TCHAR *, uint32_t);
	size_t size(void);
	size_t realsize(void);
	BOOL close(void);
	size_t read(void *, size_t, off_t = -1);
	size_t write(const void *, size_t, off_t = -1);
	BOOL seek(off_t);
	size_t _read(void *, size_t);
};

#endif //_HPCBOOT_FILE_H_
