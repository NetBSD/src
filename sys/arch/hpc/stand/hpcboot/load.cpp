/*	$NetBSD: load.cpp,v 1.10 2006/01/15 00:06:39 uwe Exp $	*/

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
#include <exec_coff.h>
#undef DPRINTF // trash coff_machdep.h 's define.

#include <console.h>
#include <memory.h>
#include <file.h>

#include <exec_elf.h>

Loader::Loader(Console *&cons, MemoryManager *&mem)
	: _mem(mem), _cons(cons)
{
	_file = 0;
	_page_tag_start = 0;
}

LoaderOps
Loader::objectFormat(File &file)
{
	union {
		Elf_Ehdr elf;
		coff_exechdr coff;
	} header;
	file.read(reinterpret_cast<void *>(&header), sizeof(header), 0);

	if (header.elf.e_ident[EI_MAG0] == ELFMAG0 &&
	    header.elf.e_ident[EI_MAG1] == ELFMAG1 &&
	    header.elf.e_ident[EI_MAG2] == ELFMAG2 &&
	    header.elf.e_ident[EI_MAG3] == ELFMAG3)
		return LOADER_ELF;
#ifdef COFF_BADMAG
	if (!COFF_BADMAG(&header.coff.f))
		return LOADER_COFF;
#endif // COFF_BADMAG

	return LOADER_UNKNOWN;
}

BOOL
Loader::loadExtData(void)
{
	size_t sz;
	vaddr_t kv;

	sz = _file->realsize();
	kv = ROUND(_kernend, static_cast <vsize_t>(KERNEL_PAGE_SIZE));

	DPRINTF((TEXT("[file system image]")));
	_load_segment(kv, sz, 0, sz);

	return _load_success();
}

void
Loader::loadEnd(void)
{
	/* tag chain end */
	_load_segment_end();
}

void
Loader::tagDump(int n)
{
#ifdef PAGE_LINK_DUMP
	struct PageTag *p, *op;
	int i = 0;

	DPRINTF((TEXT("page tag start physical address: 0x%08x\n"),
	    _page_tag_start));
	p = reinterpret_cast <struct PageTag *>(_page_tag_start);
	do  {
		if (i < n || p->src == ~0)
			DPRINTF((TEXT("[%d] next 0x%08x src 0x%08x dst 0x%08x sz 0x%x\n"),
			    i, p->next, p->src, p->dst, p->sz));
		else if (i == n)
			DPRINTF((TEXT("[...]\n")));
		op = p;
		i++;
	} while ((p = reinterpret_cast <struct PageTag *>(p->next)) != ~0);

	DPRINTF((TEXT("[%d(last)] next 0x%08x src 0x%08x dst 0x%08x sz 0x%x\n"),
	    i - 1, op->next, op->src, op->dst, op->sz));
#endif // PAGE_LINK_DUMP
}

paddr_t
Loader::tagStart(void)
{
	return _page_tag_start;
}

void
Loader::_load_segment_start(void)
{
	vaddr_t v;
	paddr_t p;

	_error = FALSE;
	_nload_link = _n0clr_link = 0;
	_tpsz = _mem->getTaggedPageSize();

	// start of chain.
	if (!_mem->getTaggedPage(v, p, &_pvec_clr, _pvec_clr_paddr))
		_error = TRUE;
#ifdef PAGE_LINK_DUMP
	_page_tag_start = (uint32_t)_pvec_clr;
#else
	_page_tag_start = _pvec_clr_paddr;
#endif
	_pvec_prev = _pvec_clr++;
	_pvec_clr_paddr += sizeof(struct PageTag);
}

void
Loader::_load_segment_end(void)
{
	_opvec_prev->next = ~0; // terminate
	DPRINTF((TEXT("load link %d zero clear link %d.\n"),
	    _nload_link, _n0clr_link));
}

void
Loader::_load_segment(vaddr_t kv, vsize_t memsz, off_t fileofs, size_t filesz)
{
	int j, n;
	vaddr_t kv_start = kv;

	DPRINTF((TEXT("\t->load 0x%08x+0x%08x=0x%08x ofs=0x%08x+0x%x\n"),
	    kv, memsz, kv + memsz, fileofs, filesz));
	_kernend = kv + memsz;

	if (filesz) {
		n = filesz / _tpsz;
		for (j = 0; j < n; j++) {
			_opvec_prev = _pvec_prev;
			_pvec_prev = _load_page(kv, fileofs,
			    _tpsz, _pvec_prev);
			kv += _tpsz;
			fileofs += _tpsz;
			++_nload_link;
		}
		size_t rest = filesz % _tpsz;
		if (rest) {
			_opvec_prev = _pvec_prev;
			_pvec_prev = _load_page(kv, fileofs, rest, _pvec_prev);
			++_nload_link;
		}
	}

	// zero clear tag
	if (filesz < memsz) {
		_pvec_prev->src = ~0;
		_pvec_prev->dst = kv_start + filesz;
		_pvec_prev->sz = memsz - filesz;
#ifdef PAGE_LINK_DUMP
		_pvec_prev->next = (uint32_t)_pvec_clr;
#else
		_pvec_prev->next = ptokv(_pvec_clr_paddr);
#endif
		DPRINTF((TEXT("\t->zero 0x%08x+0x%08x=0x%08x\n"),
		    _pvec_prev->dst, _pvec_prev->sz,
		    _pvec_prev->dst + _pvec_prev->sz));
		_opvec_prev = _pvec_prev;
		_pvec_prev = _pvec_clr++;
		_pvec_clr_paddr += sizeof(struct PageTag);
		++_n0clr_link;
	}
}

void
Loader::_load_memory(vaddr_t kv, vsize_t memsz, void *data)
{
	struct PageTag *pvec;
	paddr_t p, pvec_paddr;
	vaddr_t v;

	DPRINTF((TEXT("\t->load 0x%08x+0x%08x=0x%08x\n"),
	    kv, memsz, kv + memsz));
	if (memsz > _tpsz) {
		/* XXX failure */
		return;
	}

	_opvec_prev = _pvec_prev;
	if (!_mem->getTaggedPage(v, p, &pvec, pvec_paddr))
		_error = TRUE;
	memcpy((void *)v, data, memsz);
	_pvec_prev->src = ptokv(p);
	_pvec_prev->dst = kv;
	_pvec_prev->sz = memsz;
#ifdef PAGE_LINK_DUMP
	_pvec_prev->next = (uint32_t)pvec;
#else
	_pvec_prev->next = ptokv(pvec_paddr);
#endif
	_pvec_prev = pvec;

	_kernend = kv + memsz;
	++_nload_link;
}

struct PageTag *
Loader::_load_page(vaddr_t kv, off_t ofs, size_t sz, struct PageTag *prev)
{
	struct PageTag *pvec;
	paddr_t p, pvec_paddr;
	vaddr_t v;

	if (!_mem->getTaggedPage(v, p, &pvec, pvec_paddr))
		_error = TRUE;
	_file->read((void *)v, sz, ofs);
	prev->src = ptokv(p);
	prev->dst = kv;
	prev->sz = sz;
#ifdef PAGE_LINK_DUMP
	prev->next = (uint32_t)pvec;
#else
	prev->next = ptokv(pvec_paddr);
#endif

	return pvec;
}
