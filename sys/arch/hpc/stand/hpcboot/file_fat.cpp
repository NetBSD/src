/*	$NetBSD: file_fat.cpp,v 1.3.48.1 2006/04/22 11:37:27 simonb Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <file.h>
#include <file_fat.h>
#include <console.h>

FatFile::FatFile(Console *&cons)
	: File(cons)
{

	_debug = TRUE;
	DPRINTF((TEXT("FileManager: FAT\n")));
}

FatFile::~FatFile(void)
{

}

BOOL
FatFile::setRoot(TCHAR *drive)
{

	wcsncpy(_drive, drive, MAX_PATH);
	wcsncpy(_filename, drive, MAX_PATH);

	return TRUE;
}

BOOL
FatFile::open(const TCHAR *name, uint32_t flags)
{
	// drive + filename
	wsprintf(_filename, TEXT("%s%s"), _drive, name);

	// open it.
	_handle = CreateFile(_filename, GENERIC_READ, 0, 0,
	    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (_handle == INVALID_HANDLE_VALUE) {
		DPRINTF((TEXT("can't open \"%s\". cause = %d\n"),
		    _filename, GetLastError()));
		return FALSE;
	}

	size_t sz = size();
	if (sz == ~0) {
		DPRINTF((TEXT("can't get file size.\n")));
		CloseHandle(_handle);
		return FALSE;
	}
	DPRINTF((TEXT("open file \"%s\"(%d byte).\n"), _filename, sz));

	return TRUE;
}

size_t
FatFile::read(void *buf, size_t bytes, off_t ofs)
{
	unsigned long readed;

	if (ofs != -1)
		SetFilePointer(_handle, ofs, 0, FILE_BEGIN);
	ReadFile(_handle, buf, bytes, &readed, 0);

	return size_t(readed);
}

size_t
FatFile::write(const void *buf, size_t bytes, off_t ofs)
{
	unsigned long wrote;

	if (ofs != -1)
		SetFilePointer(_handle, ofs, 0, FILE_BEGIN);
	WriteFile(_handle, buf, bytes, &wrote, 0);

	return size_t(wrote);
}

BOOL
FatFile::seek(off_t ofs)
{

	SetFilePointer(_handle, ofs, 0, FILE_BEGIN);

	return TRUE;
}
