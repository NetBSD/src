/*	$NetBSD: load_coff.cpp,v 1.2 2001/05/08 18:51:23 uch Exp $	*/

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

#include <load.h>
#include <load_coff.h>
#undef DPRINTF // trash coff_machdep.h 's define.
#include <console.h>
#include <memory.h>
#include <file.h>

CoffLoader::CoffLoader(Console *&cons, MemoryManager *&mem)
	: Loader(cons, mem)
{
	memset(&_eh, 0, sizeof(struct coff_exechdr));
	_fh = &_eh.f;
	_ah = &_eh.a;

	DPRINTF((TEXT("Loader: COFF\n")));
}

CoffLoader::~CoffLoader(void)
{
}

BOOL
CoffLoader::setFile(File *&file)
{
	Loader::setFile(file);

	/* read COFF header and check it */
	return read_header();
}

size_t
CoffLoader::memorySize()
{
	size_t sz = _ah->a_tsize + _ah->a_dsize;

	DPRINTF((TEXT("file size: text 0x%x + data 0x%x = 0x%x byte\n"),
	    _ah->a_tsize, _ah->a_dsize, sz));
	return sz;
}

kaddr_t
CoffLoader::jumpAddr()
{
	DPRINTF((TEXT("kernel entry address: 0x%08x\n"), _ah->a_entry));
	return _ah->a_entry;
}

BOOL
CoffLoader::load()
{
	size_t filesz;
	size_t memsz;
	vaddr_t kv;
	off_t fileofs;
  
	/* start tag chain */
	_load_segment_start();

	/* text */
	filesz = memsz = _ah->a_tsize;
	kv = _ah->a_tstart;
	fileofs = COFF_ROUND(N_TXTOFF(_fh, _ah), 16);
	DPRINTF((TEXT("[text]")));
	_load_segment(kv, memsz, fileofs, filesz);
	/* data */
	fileofs += filesz;
	filesz = memsz = _ah->a_dsize;
	kv = _ah->a_dstart;
	DPRINTF((TEXT("[data]")));
	_load_segment(kv, memsz, fileofs, filesz);
	/* bss */
	filesz = 0;
	memsz = _ah->a_bsize;
	kv = _ah->a_dstart + _ah->a_dsize;
	fileofs = 0;
	DPRINTF((TEXT("[bss ]")));
	_load_segment(kv, memsz, fileofs, filesz);

	/* tag chain still opening */

	return TRUE;
}

BOOL
CoffLoader::read_header(void)
{
#ifndef COFF_BADMAG
	DPRINTF((TEXT("coff loader not implemented.\n")));
	return FALSE;
#else
	// read COFF header
	_file->read(&_eh, sizeof(struct coff_exechdr), 0);

	// check COFF Magic.
	if (COFF_BADMAG(_fh)) {
		DPRINTF((TEXT("not a COFF file.\n")));
		return FALSE;
	}

	return TRUE;
#endif
}
