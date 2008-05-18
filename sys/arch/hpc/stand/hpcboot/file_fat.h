/* -*-C++-*-	$NetBSD: file_fat.h,v 1.4.70.1 2008/05/18 12:31:59 yamt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#ifndef _HPCBOOT_FILE_FAT_H_
#define	_HPCBOOT_FILE_FAT_H_

#include <file.h>

class FatFile : public File {
private:
	HANDLE _handle;
	TCHAR _drive[MAX_PATH];	// drive.
	TCHAR _filename[MAX_PATH];

public:
	FatFile(Console *&);
	virtual ~FatFile(void);

	BOOL setRoot(TCHAR *);
	BOOL open(const TCHAR *, uint32_t);
	size_t size(void) { return GetFileSize(_handle, 0); }
	BOOL close(void) { return CloseHandle(_handle); }
	size_t read(void *, size_t, off_t = -1);
	size_t write(const void *, size_t, off_t = -1);
	BOOL seek(off_t);
};

#endif //_HPCBOOT_FILE_FAT_H_
