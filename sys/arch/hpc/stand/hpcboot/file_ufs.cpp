/*	$NetBSD: file_ufs.cpp,v 1.4.34.1 2012/04/17 00:06:23 yamt Exp $	*/

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

#include <machine/cdefs.h>
#include <machine/types.h>
#include <machine/int_types.h>

#include <file.h>
#include <file_ufs.h>
#include <console.h>

__BEGIN_DECLS
#include <stand.h>
#include <lib/libsa/ufs.h>
#include <winblk.h>
__END_DECLS

UfsFile::UfsFile(Console *&cons)
	: File(cons)
{
	static struct devsw devsw[] = {
		{"winblk", winblkstrategy, winblkopen,
		 winblkclose, winblkioctl },
	};

	_f = static_cast<struct open_file *>(malloc(sizeof(struct open_file)));
	_f->f_dev = devsw;
	_debug = TRUE;

	DPRINTF((TEXT("FileManager: UFS\n")));
}

UfsFile::~UfsFile(void)
{
	free(_f);
}

BOOL
UfsFile::setRoot(TCHAR *drive)
{
	char name[MAX_PATH];
	char devname[4] = "DSK";
	int unit = 1;

	_to_ascii(name, drive, MAX_PATH);
	if ('1' <= name[0] && name[0] <= '9' && name[1] == ':')
		unit = name[0] - '0';
	else if (isalpha((unsigned char)name[0]) &&
	    isalpha((unsigned char)name[1]) &&
	    isalpha((unsigned char)name[2]) &&
	    '1' <= name[3] && name[3] <= '9' && name[4] == ':') {
		devname[0] = name[0];
		devname[1] = name[1];
		devname[2] = name[2];
		unit = name[3] - '0';
	}

	winblkopen(_f, devname, unit, 0);

	return TRUE;
}

BOOL
UfsFile::open(const TCHAR *name, uint32_t flags)
{
	int error;

	if (!_to_ascii(_ascii_filename, name, MAX_PATH))
		return FALSE;

	error = ufs_open(_ascii_filename, _f);
	DPRINTF((TEXT("open file \"%s\" "), name));

	if (!error)
		DPRINTF((TEXT("(%d byte).\n"), size()));
	else
		DPRINTF((TEXT("failed.\n")));

	return !error;
}

size_t
UfsFile::read(void *buf, size_t bytes, off_t ofs)
{
	size_t sz;

	if (ofs != -1)
		ufs_seek(_f, ofs, SEEK_SET);
	ufs_read(_f, buf, bytes, &sz);

	return bytes - sz;
}

size_t
UfsFile::write(const void *buf, size_t bytes, off_t ofs)
{
	size_t sz;

	if (ofs != -1)
		ufs_seek(_f, ofs, SEEK_SET);
	ufs_write(_f,(void *) buf, bytes, &sz);

	return bytes - sz;
}

BOOL
UfsFile::seek(off_t ofs)
{
	ufs_seek(_f, ofs, SEEK_SET);

	return TRUE;
}

size_t
UfsFile::size()
{
	struct stat st;

	ufs_stat(_f, &st);

	return st.st_size;
}

BOOL
UfsFile::close()
{
	return ufs_close(_f);
}
