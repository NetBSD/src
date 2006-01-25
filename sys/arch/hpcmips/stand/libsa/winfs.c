/*	$NetBSD: winfs.c,v 1.3 2006/01/25 18:28:26 christos Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura.
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#define STANDALONE_WINDOWS_SIDE
#include <stand.h>
#include <winfs.h>

#define MAXPATHLEN 1024

/*
 *  file system specific context.
 */
struct winfs {
	HANDLE	hDevice;
};


int 
win_open(path, f)
	char           *path;
	struct open_file *f;
{
	TCHAR *wpath = (TCHAR*)path;
	struct winfs *fsdata;

	fsdata = (struct winfs *)alloc(sizeof(*fsdata));
	if (!fsdata) {
		return (ENOMEM);
	}

	win_printf(TEXT("open(%s)\n"), wpath);
	fsdata->hDevice = CreateFile(wpath, GENERIC_READ, 0, NULL,    
				    OPEN_EXISTING, 0, NULL);
	if (fsdata->hDevice == INVALID_HANDLE_VALUE) {
		win_printf(TEXT("can't open %s.\n"), wpath);
		dealloc(fsdata, sizeof(*fsdata));
		return (EIO);	/* XXX, We shuld check GetLastError(). */
	}

	f->f_fsdata = (void *)fsdata;

	return (0);
}


int 
win_close(f)
	struct open_file *f;
{
	struct winfs *fsdata = (struct winfs *) f->f_fsdata;

	if (fsdata->hDevice != INVALID_HANDLE_VALUE) {
		CloseHandle(fsdata->hDevice);
	}
	dealloc(fsdata, sizeof(*fsdata));

	return (0);
}


int 
win_read(f, addr, size, resid)
	struct open_file *f;
	void           *addr;
	size_t         size;
	size_t         *resid;	/* out */
{
	struct winfs *fsdata = (struct winfs *) f->f_fsdata;
	DWORD read_len;

	while (size > 0) {
		if (!ReadFile(fsdata->hDevice,
			      (u_char*)addr, size,
			      &read_len, NULL)) {
			win_printf(TEXT("ReadFile() failed.\n"));
		}
		
		if (read_len == 0)
			break;	/* EOF */

		(unsigned long)addr += read_len;
		size -= read_len;
	}

	if (resid)
		*resid = size;
	return (0);
}

int 
win_write(f, start, size, resid)
	struct open_file *f;
	void           *start;
	size_t          size;
	size_t         *resid;	/* out */
{
	return (EROFS);	/* XXX */
}


int 
win_stat(f, sb)
	struct open_file *f;
	struct stat    *sb;
{
	sb->st_mode = 0444;
	sb->st_nlink = 1;
	sb->st_uid = 0;
	sb->st_gid = 0;
	sb->st_size = -1;
	return (0);
}

off_t 
win_seek(f, offset, whence)
	struct open_file *f;
	off_t           offset;
	int             whence;
{
	struct winfs *fsdata = (struct winfs *) f->f_fsdata;
	DWORD dwPointer;
	int winwhence;

	switch (whence) {
	case SEEK_SET:
		winwhence = FILE_BEGIN;
		break;
	case SEEK_CUR:
		winwhence = FILE_CURRENT;
		break;
	case SEEK_END:
		winwhence = FILE_END;
		break;
	default:
		errno = EOFFSET;
		return (-1);
	}

	dwPointer = SetFilePointer(fsdata->hDevice, offset, NULL, winwhence);
	if (dwPointer == 0xffffffff) {
		errno = EINVAL;	/* XXX, We shuld check GetLastError(). */
		return (-1);
	}

	return (0);
}
