/* -*-C++-*-	$NetBSD: memory.h,v 1.3 2001/05/08 18:51:23 uch Exp $	*/

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

#ifndef _HPCBOOT_MEMORY_H_
#define _HPCBOOT_MEMORY_H_

#undef MEMORY_MAP_DEBUG		// super verbose.

#include <hpcboot.h>
class Console;

template <class T>
inline T
ROUND(const T v, const T x) {
	return((v + x - 1) / x) * x;
}

template <class T>
inline T
TRUNC(const T v, const T x) {
	return(v / x) * x;
}

#define MAX_MEM_BANK	16
class MemoryManager {
private:
	struct bank {
		paddr_t addr;
		psize_t size;
	};
	// Windows CE application virtual memory size
	enum { WCE_VMEM_MAX = 32 * 1024 * 1024 };
	// Windows CE VirtualAlloc() boundary
	enum { WCE_REGION_SIZE = 64 * 1024 };

protected:
	// Console options.
	Console *&_cons;
	BOOL _debug;
  
	enum { BLOCK_SIZE = WCE_REGION_SIZE * 64 }; // 4MByte

	// Pagesize, D-RAM bank
	vsize_t _page_size;
	int _page_shift;
	int _page_per_region;
	struct bank _bank[MAX_MEM_BANK];
	int _nbank;

	// Allocated memory
	vaddr_t _memory;

	// Virtual <-> Physical table
	int _naddr_table; 
	struct AddressTranslationTable {
		vaddr_t vaddr;
		paddr_t paddr;
	}
	*_addr_table;
	int _addr_table_idx;

public:
	vsize_t getPageSize(void) { return _page_size; }

	vsize_t getTaggedPageSize(void)
		{ return _page_size - sizeof(struct PageTag); }
	vsize_t estimateTaggedPageSize(vsize_t sz) {
		vsize_t tsz = getTaggedPageSize();
		return((sz + tsz - 1) / tsz) * _page_size;
	}
	u_int32_t roundPage(u_int32_t v) { return ROUND(v, _page_size); }
	u_int32_t truncPage(u_int32_t v) { return TRUNC(v, _page_size); }
	u_int32_t roundRegion(u_int32_t v)
		{ return ROUND(v, u_int32_t(WCE_REGION_SIZE)); }
	u_int32_t truncRegion(u_int32_t v)
		{ return TRUNC(v, u_int32_t(WCE_REGION_SIZE)); }

	// Physical address ops.
	vaddr_t mapPhysicalPage(paddr_t paddr, psize_t size, u_int32_t flags);
	void unmapPhysicalPage(vaddr_t vaddr);
	u_int32_t readPhysical4(paddr_t paddr);
	// return physical address of coresspoing virtual address.
	virtual paddr_t searchPage(vaddr_t vaddr) = 0;

	MemoryManager(Console *&cons, size_t pagesize);
	virtual ~MemoryManager(void);
	BOOL &setDebug(void) { return _debug; }
	virtual BOOL init(void) { return TRUE; }

	// initialize page pool
	BOOL reservePage(vsize_t size, BOOL page_commit = FALSE);
	// register D-RAM banks
	void loadBank(paddr_t paddr, psize_t psize);
	// get 1 page from high address of pool.
	BOOL getPage(vaddr_t &vaddr, paddr_t &paddr);
	// get tagged page from low address of pool.
	BOOL getTaggedPage(vaddr_t &vaddr, paddr_t &paddr);
	BOOL getTaggedPage(vaddr_t &v, paddr_t &p, struct PageTag **pvec,
	    paddr_t &pvec_paddr);
};

//
//	Use LockPages()
//
class MemoryManager_LockPages : public MemoryManager {
private:
	BOOL(*_lock_pages)(LPVOID, DWORD, PDWORD, int);
	BOOL(*_unlock_pages)(LPVOID, DWORD);
	int _shift;

public:
	MemoryManager_LockPages(BOOL(*)(LPVOID, DWORD, PDWORD, int),
	    BOOL(*)(LPVOID, DWORD), Console *&, size_t, int = 0);
	virtual ~MemoryManager_LockPages(void);
	paddr_t searchPage(vaddr_t vaddr);
};

//
//	Use VirtualCopy()
//
class MemoryManager_VirtualCopy : public MemoryManager {
private:
	// search guess
	paddr_t _search_guess;
	
	// Memory marker
	u_int32_t _magic0, _magic1;
	volatile u_int32_t *_magic_addr;
	enum {
		MAGIC_CHECK_DONE	= 0xac1dcafe, 
		MAGIC_CHECK_DUMMY	= 0xa5a5a5a5
	};
	void setMagic(vaddr_t v) {
		_magic_addr =(u_int32_t *)v;
		while ((_magic0 = Random()) == MAGIC_CHECK_DONE)
			;
		while ((_magic1 = Random()) == MAGIC_CHECK_DONE)
			;
		_magic_addr[0] = _magic0;
		_magic_addr[1] = _magic1;
	}
	BOOL checkMagic(vaddr_t v) {
		volatile u_int32_t *marker =(u_int32_t *)v;
		return (marker[0] == _magic0) && (marker[1] == _magic1); 
	}
	void clearMagic(void) {
		_magic_addr[0] = MAGIC_CHECK_DONE;
		_magic_addr[1] = MAGIC_CHECK_DONE;
	}
	vaddr_t checkMagicRegion(vaddr_t start, vsize_t size,
	    vsize_t step = 8) {
		for (vaddr_t ref = start; ref < start + size; ref += step)
			if (checkMagic(ref))
				return ref - start;
		return ~0;
	}
	paddr_t searchBank(int banknum);

public:
	MemoryManager_VirtualCopy(Console *&, size_t);
	virtual ~MemoryManager_VirtualCopy(void);
	paddr_t searchPage(vaddr_t vaddr);
};

#endif // _HPCBOOT_MEMORY_H_
