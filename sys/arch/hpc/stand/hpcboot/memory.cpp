/*	$NetBSD: memory.cpp,v 1.5 2002/02/04 17:32:02 uch Exp $	*/

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

#include <memory.h>
#include <console.h>

MemoryManager::MemoryManager(Console *&cons, size_t pagesize)
	: _cons(cons)
{
	_debug = FALSE;
	_page_size = pagesize;
  
	int mask = _page_size;
	for (_page_shift = 0; !(mask & 1); _page_shift++)
		mask >>= 1;

	_page_per_region = WCE_REGION_SIZE / _page_size;
	_nbank = 0;
	_addr_table_idx = 0;
	_addr_table = 0;
	_memory = 0;
}

MemoryManager::~MemoryManager(void)
{
	if (_memory)
		VirtualFree(LPVOID(_memory), 0, MEM_RELEASE);
}

void
MemoryManager::loadBank(paddr_t paddr, psize_t psize)
{
	struct MemoryManager::bank *b = &_bank[_nbank++];
	b->addr = paddr;
	b->size = psize;
	DPRINTF((TEXT("[%d] 0x%08x size 0x%08x\n"), _nbank - 1,
	    b->addr, b->size));
}

BOOL
MemoryManager::reservePage(vsize_t size, BOOL page_commit)
{
	// My virtual memory space
	vaddr_t vbase;
	vsize_t vsize;

	int i, npage;
    
	if (size == 0)
		return FALSE;

	// reserve all virtual memory.
	vsize = roundRegion(size);
	npage = roundPage(size) / _page_size;

	size_t tabsz = sizeof(struct AddressTranslationTable) * npage;
	_addr_table = static_cast <struct AddressTranslationTable *>
	    (malloc(tabsz));
	if (_addr_table == NULL) {
		DPRINTF((TEXT("can't allocate memory for translation table.\n")));
		return FALSE;
	}
	DPRINTF((TEXT("address translation table %d pages.(%d byte)\n"), npage,
	    tabsz));

	if (page_commit)
		vbase = vaddr_t(VirtualAlloc(0, vsize, MEM_RESERVE,
		    PAGE_NOACCESS));
	else
		vbase = vaddr_t(VirtualAlloc(0, vsize, MEM_COMMIT,
		    PAGE_READWRITE | PAGE_NOCACHE));

	if (vbase == 0) {
		DPRINTF((TEXT("can't allocate memory\n")));
		return FALSE;
	}
	_memory = vbase;

	// find physical address of allocated page.
	AddressTranslationTable *tab = _addr_table;
	_naddr_table = 0;
	for (i = 0; i < npage; i++) {
		vaddr_t vaddr;
		paddr_t paddr = ~0;

		if (page_commit)
			// now map to physical page.
			vaddr = vaddr_t(VirtualAlloc(
				LPVOID(vbase + _page_size * i),
				_page_size, MEM_COMMIT,
				PAGE_READWRITE | PAGE_NOCACHE));
		else
			vaddr = vbase + _page_size * i;

		paddr = searchPage(vaddr);

		if (paddr == ~0) {
			DPRINTF((TEXT("page#%d not found\n"), i));
			break;
		} else {
#ifdef MEMORY_MAP_DEBUG
			DPRINTF((TEXT("page %d vaddr=0x%08x paddr=0x%08x\n"),
			    _naddr_table, vaddr, paddr));
#endif
			tab->vaddr = vaddr;
			tab->paddr = paddr;
			++tab;
			++_naddr_table;
		}
	}

#ifdef MEMORY_MAP_DEBUG
	// dump virtual <-> physical address table
	tab = _addr_table;
	for (i = 0; i < _naddr_table;) {
		for (int j = 0; j < 4; j++, i++, tab++)
			DPRINTF((TEXT("%08x=%08x "), tab->vaddr, tab->paddr));
		DPRINTF((TEXT("\n")));
	}
#endif
	DPRINTF((TEXT("allocated %d page. mapped %d page.\n"), npage,
	    _naddr_table));

	return TRUE;
}

BOOL
MemoryManager::getPage(vaddr_t &vaddr, paddr_t &paddr)
{
	/* get plain page from the top */
	if (_addr_table_idx >= _naddr_table ||
	    _addr_table == NULL)
		return FALSE;

	int idx = --_naddr_table;

	AddressTranslationTable *tab = &_addr_table[idx];
	vaddr = tab->vaddr;
	paddr = tab->paddr;

	return TRUE;
}

BOOL
MemoryManager::getTaggedPage(vaddr_t &vaddr, paddr_t &paddr)
{
	/* get tagged page from the bottom */
	if (_addr_table_idx >= _naddr_table ||
	    _addr_table == NULL) {
		DPRINTF((TEXT("page insufficient.\n")));
		return FALSE;
	}
	AddressTranslationTable *tab =
	    &_addr_table[_addr_table_idx++];  
	vaddr = tab->vaddr;
	paddr = tab->paddr;
  
	return TRUE;
}

BOOL 
MemoryManager::getTaggedPage(vaddr_t &v, paddr_t &p,
    struct PageTag **pvec, paddr_t &pvec_paddr)
{
	if (!getTaggedPage(v, p))
		return FALSE;
    
	*pvec =(struct PageTag *)v;
	memset(*pvec, 0, sizeof(struct PageTag));
	v += sizeof(struct PageTag);
	pvec_paddr = p;
	p += sizeof(struct PageTag);

	return TRUE;
}

vaddr_t
MemoryManager::mapPhysicalPage(paddr_t paddr, psize_t size, u_int32_t flags)
{
	paddr_t pstart = truncPage(paddr);
	paddr_t pend = roundPage(paddr + size);
	psize_t psize = pend - pstart;

	LPVOID p = VirtualAlloc(0, psize, MEM_RESERVE, PAGE_NOACCESS);

	int ok = VirtualCopy(p, LPVOID(pstart >> 8), psize,
	    flags | PAGE_NOCACHE | PAGE_PHYSICAL);
	if (!ok) {
		DPRINTF((TEXT("can't map physical address 0x%08x\n"), paddr));
		return ~0;
	}
#if 0
	DPRINTF((TEXT("start=0x%08x end=0x%08x size=0x%08x return=0x%08x\n"),
	    pstart, pend, psize, vaddr_t(p) + vaddr_t(paddr - pstart)));
	    
#endif
  
	return vaddr_t(p) + vaddr_t(paddr - pstart);
}

void
MemoryManager::unmapPhysicalPage(vaddr_t vaddr)
{
	int ok = VirtualFree(LPVOID(truncPage(vaddr)), 0, MEM_RELEASE);
	if (!ok)
		DPRINTF((TEXT("can't release memory\n")));
}

u_int32_t
MemoryManager::readPhysical4(paddr_t paddr)
{
	vaddr_t v = mapPhysicalPage(paddr, 4, PAGE_READONLY);
	u_int32_t val = *(u_int32_t *)v;
	unmapPhysicalPage(v);
	return val;
}

//
//	Use LockPages()
//
MemoryManager_LockPages::MemoryManager_LockPages
(BOOL(*lock_pages)(LPVOID, DWORD, PDWORD, int),
    BOOL(*unlock_pages)(LPVOID, DWORD),
    Console *&cons, size_t pagesize, int shift)
	:  MemoryManager(cons, pagesize)
{
	_lock_pages	= lock_pages;
	_unlock_pages	= unlock_pages;
	_shift = shift;
	DPRINTF((TEXT("MemoryManager: LockPages\n")));
}

MemoryManager_LockPages::~MemoryManager_LockPages(void)
{
}

paddr_t
MemoryManager_LockPages::searchPage(vaddr_t vaddr)
{
	paddr_t paddr = ~0;

	if (!_lock_pages(LPVOID(vaddr), _page_size, PDWORD(&paddr), 1))  
		return paddr;

	if (!_unlock_pages(LPVOID(vaddr), _page_size)) {
		DPRINTF((TEXT("can't unlock pages\n")));
	}
  
	return(paddr >>(_page_shift - _shift)) << _page_shift;
}

//
//	Use VirtualCopy()
//
MemoryManager_VirtualCopy::MemoryManager_VirtualCopy(Console *&cons,
    size_t pagesize) 
	: MemoryManager(cons, pagesize)
{
	_search_guess = 0;
	DPRINTF((TEXT("MemoryManager: VirtualCopy\n")));
}

MemoryManager_VirtualCopy::~MemoryManager_VirtualCopy(void)
{
}

paddr_t
MemoryManager_VirtualCopy::searchPage(vaddr_t vaddr)
{
	paddr_t paddr = ~0;
	int i;

	// search all D-RAM bank.
	setMagic(vaddr);
 retry:
	for (i = 0; i < _nbank; i++) {
		paddr = searchBank(i);
		if (paddr != ~0)
			break;
	}
	if (_search_guess != 0 && paddr == ~0) {
		_search_guess = 0;
		goto retry;
	}

	clearMagic();

	return paddr;
}

paddr_t
MemoryManager_VirtualCopy::searchBank(int banknum)
{
	LPVOID ref;
	paddr_t paddr, pstart, pend, pfound = ~0;
	paddr_t bstart, bend;
	vaddr_t ofs;

	bstart = _bank[banknum].addr;
	bend = _bank[banknum].addr + _bank[banknum].size;

	pstart = _search_guess ? _search_guess : bstart;
	pend = bend;

	if (pstart < bstart || pstart >= pend)
		return pfound;

	// reserve physical reference region
	ref = VirtualAlloc(0, BLOCK_SIZE, MEM_RESERVE, PAGE_NOACCESS);
	if (ref == 0) {
		DPRINTF((TEXT("can't allocate virtual memory.\n")));
		return pfound;
	}

	for (paddr = pstart; paddr < pend; paddr += BLOCK_SIZE) {
		if (!VirtualCopy(ref, LPVOID(paddr >> 8), BLOCK_SIZE,
		    PAGE_READONLY | PAGE_NOCACHE | PAGE_PHYSICAL)) {
			DPRINTF((TEXT("can't map physical addr 0x%08x(->0x%08x)\n"),
			    ref, paddr));
			goto release;
		}

		// search magic in this region. 
		ofs = checkMagicRegion(vaddr_t(ref), BLOCK_SIZE, _page_size);

		// decommit reference region.
		if (!VirtualFree(ref, BLOCK_SIZE, MEM_DECOMMIT)) {
			DPRINTF((TEXT("can't decommit addr 0x%08x(->0x%08x)\n"),
			    ref, paddr));
			goto release;
		}

		if (ofs != ~0) {
			pfound = paddr + ofs;
			_search_guess = paddr;
			break;
		}
	}
 release:
	if (!VirtualFree(ref, 0, MEM_RELEASE))
		DPRINTF((TEXT("can't release memory\n")));

	return pfound;
}
