/* -*-C++-*-	$NetBSD: load.h,v 1.2.2.1 2002/03/16 15:57:50 jdolecek Exp $	*/

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

#ifndef _HPCBOOT_LOAD_H_
#define _HPCBOOT_LOAD_H_

#include <hpcboot.h>
#include <machine/endian_machdep.h>

#undef PAGE_LINK_DUMP	// debug use only. if defined, you can't boot.
class Console;
class MemoryManager;
class File;

class Loader {
private:
	struct PageTag *_pvec_prev, *_opvec_prev, *_pvec_clr;
	paddr_t _pvec_clr_paddr;
	int _nload_link, _n0clr_link;
	size_t _tpsz;
	struct PageTag *_load_page(vaddr_t, off_t, size_t, struct PageTag *);
	BOOL _error;

protected:
	BOOL		_debug;
	Console	*&_cons;
	MemoryManager	*&_mem;
	File		*_file;

	paddr_t _page_tag_start;
	paddr_t _kernend;

	Loader(Console *&, MemoryManager *&);

	void _load_segment_start(void);
	void _load_segment(vaddr_t, vsize_t, off_t, size_t);
	void _load_memory(vaddr_t, vsize_t, void *);
	void _load_segment_end(void);
	BOOL _load_success(void) const { return !_error; };

public:
	virtual ~Loader(void) { /* NO-OP */ }

	BOOL &setDebug(void) { return _debug; }

	virtual BOOL setFile(File *&file) { _file = file; return TRUE; }
	virtual size_t memorySize(void) = 0;
	virtual BOOL load(void) = 0;
	virtual kaddr_t jumpAddr(void) = 0;

	paddr_t tagStart(void);
	BOOL loadExtData(void);
	void loadEnd(void);

	void tagDump(int);	// for debug

	static LoaderOps objectFormat(File &);
};

#endif //_HPCBOOT_LOAD_H_
