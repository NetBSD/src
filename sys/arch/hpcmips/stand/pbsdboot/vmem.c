/*	$NetBSD: vmem.c,v 1.3 1999/09/26 02:42:52 takemura Exp $	*/

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
#include <pbsdboot.h>

struct addr_s {
	caddr_t addr;
	int in_use;
};

struct page_header_s {
	unsigned long magic0;
	int pageno;
	unsigned long magic1;
};

struct map_s *map = NULL;
struct addr_s *phys_addrs = NULL;
unsigned char* heap = NULL;
int npages;
caddr_t kernel_start;
caddr_t kernel_end;

int
vmem_exec(caddr_t entry, int argc, char *argv[], struct bootinfo *bi)
{
	int i;
	caddr_t p;

	if (map == NULL) {
		debug_printf(TEXT("vmem is not initialized.\n"));
		msg_printf(MSG_ERROR, whoami, TEXT("vmem is not initialized.\n"));
		return (-1);
	}

	debug_printf(TEXT("entry point=0x%x\n"), entry);

	map->entry = entry;
	map->base = kernel_start;

	for (i = 0; i < argc; i++) {
		argv[i] = vtophysaddr(argv[i]);
	}
	map->arg0 = (caddr_t)argc;
	map->arg1 = vtophysaddr((caddr_t)argv);
	map->arg2 = vtophysaddr((caddr_t)bi);
	map->arg3 = NULL;

	if (map->arg1 == NULL || map->arg2 == NULL) {
		debug_printf(TEXT("arg, vtophysaddr() failed\n"));
		msg_printf(MSG_ERROR, whoami,
			   TEXT("arg, vtophysaddr() failed\n"));
		return (-1);
	}

	for (i = 0; p = map->leaf[i / map->leafsize][i % map->leafsize]; i++)  {
		if ((p = vtophysaddr(p)) == NULL) {
			debug_printf(TEXT("vtophysaddr() failed, page %d (addr=0x%x) \n"),
				     i, map->leaf[i / map->leafsize][i % map->leafsize]);
			msg_printf(MSG_ERROR, whoami,
				   TEXT("vtophysaddr() failed, page %d (addr=0x%x) \n"),
				   i, map->leaf[i / map->leafsize][i % map->leafsize]);
			return (-1);
		}
		map->leaf[i / map->leafsize][i % map->leafsize] = p;
	}

	for (i = 0; i < map->nleaves; i++) {
		if ((p = vtophysaddr((caddr_t)map->leaf[i])) == NULL) {
			debug_printf(TEXT("vtophysaddr() failed, leaf %d (addr=0x%x) \n"),
				     i, map->leaf[i / map->leafsize][i % map->leafsize]);
			msg_printf(MSG_ERROR, whoami,
				   TEXT("vtophysaddr() failed, leaf %d (addr=0x%x) \n"),
				   i, map->leaf[i / map->leafsize][i % map->leafsize]);
			return (-1);
		}
		map->leaf[i] = (caddr_t*)p;
	}

	debug_printf(TEXT("execute startprog()\n"));
	//return (-1);
	return ((*system_info.si_boot)(vtophysaddr((caddr_t)map)));
}

caddr_t
vmem_alloc()
{
	int i, pagesize;
	struct page_header_s *page;

	pagesize = system_info.si_pagesize;
	for (i = 0; i < npages; i++) {
		page = (struct page_header_s*)&heap[pagesize * i];
		if (!phys_addrs[i].in_use &&
		    !(kernel_start <= phys_addrs[i].addr &&
		      phys_addrs[i].addr < kernel_end)) {
			phys_addrs[i].in_use = 1;
			return ((caddr_t)page);
		}
	}
	return (NULL);
}

static caddr_t
alloc_kpage(caddr_t phys_addr)
{
	int i, pagesize;
	struct page_header_s *page;

	pagesize = system_info.si_pagesize;
	for (i = 0; i < npages; i++) {
		page = (struct page_header_s*)&heap[pagesize * i];
		if (phys_addrs[i].addr == phys_addr) {
			if (phys_addrs[i].in_use) {
				debug_printf(TEXT("page %d (phys addr=0x%x) is already in use\n"),
					     i, phys_addr);
				msg_printf(MSG_ERROR, whoami,
					   TEXT("page %d (phys addr=0x%x) is already in use\n"),
					   i, phys_addr);
				return (NULL);
			}
			phys_addrs[i].in_use = 1;
			return ((caddr_t)page);
		}
	}
	return (vmem_alloc());
}

caddr_t
vmem_get(caddr_t phys_addr, int *length)
{
	int pagesize = system_info.si_pagesize;
	int pageno = (phys_addr - kernel_start) / pagesize;
	int offset = (phys_addr - kernel_start) % pagesize;

	if (map == NULL || pageno < 0 || npages <= pageno) {
		return (NULL);
	}
	if (length) {
		*length = pagesize - offset;
	}
	return (map->leaf[pageno / map->leafsize][pageno % map->leafsize] + offset);
}

caddr_t
vtophysaddr(caddr_t page)
{
	int pageno = (page - heap) / system_info.si_pagesize;
	int offset = (page - heap) % system_info.si_pagesize;

	if (map == NULL || pageno < 0 || npages <= pageno) {
		return (NULL);
	}
	return (phys_addrs[pageno].addr + offset);
}

int
vmem_init(caddr_t start, caddr_t end)
{
#define MEM_BLOCK_SIZE (1024*1024*4) /* must be greater than page size */
	int i, m, pageno;
	unsigned long magic0;
	unsigned long magic1;
	int nfounds;
	struct page_header_s *page;
	long size;
	int nleaves;
	int pagesize, memblocks;

	pagesize = system_info.si_pagesize;
	memblocks = (system_info.si_drammaxsize) / MEM_BLOCK_SIZE;

	/* align with page size */
	start = (caddr_t)(((long)start / pagesize) * pagesize);
	end = (caddr_t)((((long)end + pagesize - 1) / pagesize) * pagesize);

	kernel_start = start;
	kernel_end = end;
	size = end - start;

	/*
	 *  program image pages.
	 */
	npages = (size + pagesize - 1) / pagesize;

	/*
	 *  map leaf pages.
	 *  npages plus one for end mark.
	 */
	npages += (nleaves = ((npages * sizeof(caddr_t) + pagesize) / pagesize));

	/*
	 *  map root page, startprg code page, argument page and bootinfo page.
	 */
	npages += 4;

	/*
	 *  allocate pages
	 */
	debug_printf(TEXT("allocate %d pages\n"), npages);
	heap = (unsigned char*)
		VirtualAlloc(0,
			     npages * pagesize,
			     MEM_COMMIT,
			     PAGE_READWRITE | PAGE_NOCACHE);
	if (heap == NULL) {
		debug_printf(TEXT("can't allocate heap\n"));
		msg_printf(MSG_ERROR, whoami, TEXT("can't allocate heap\n"));
		goto error_cleanup;
	}

	/*
	 *  allocate address table.
	 */
	phys_addrs = (struct addr_s *)
		VirtualAlloc(0,
			     npages * sizeof(struct addr_s),
			     MEM_COMMIT,
			     PAGE_READWRITE);
	if (phys_addrs == NULL) {
		debug_printf(TEXT("can't allocate address table\n"));
		msg_printf(MSG_ERROR, whoami, TEXT("can't allocate address table\n"));
		goto error_cleanup;
	}

	/*
	 *  set magic number for each page in buffer.
	 */
	magic0 = Random();
	magic1 = Random();
	debug_printf(TEXT("magic=%08x%08x\n"), magic0, magic1);

	for (i = 0; i < npages; i++) {
		page = (struct page_header_s*)&heap[pagesize * i];
		page->magic0 = magic0;
		page->pageno = i;
		page->magic1 = magic1;
		phys_addrs[i].addr = 0;
		phys_addrs[i].in_use = 0;
	}

	/*
	 *  Scan whole physical memory.
	 */
	nfounds = 0;
	for (m = 0; (m < memblocks) && (nfounds < npages); m++) {
		unsigned char* mem;
		/* Map physical memory block */
		mem = (unsigned char*)VirtualAlloc(0, MEM_BLOCK_SIZE, 
						   MEM_RESERVE, PAGE_NOACCESS);
		if(!VirtualCopy((LPVOID)mem, (LPVOID)
				((system_info.si_dramstart + MEM_BLOCK_SIZE * m) >> 8),
				MEM_BLOCK_SIZE,	
				PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL)) {
			VirtualFree(mem, 0, MEM_RELEASE);			
			continue;
		}
		/* Find preliminary allocated pages */
		for (i = 0; i < (int)(MEM_BLOCK_SIZE / pagesize); i++) {
			page = (struct page_header_s*)&mem[pagesize * i];
			if (page->magic0 == magic0 &&
			    page->magic1 == magic1) {
				pageno = page->pageno;
				if (0 <= pageno && pageno < npages &&
				    phys_addrs[pageno].addr == 0) {
					/* Set kernel virtual addr. XXX mips dependent */
					phys_addrs[pageno].addr = (unsigned char*)
						((0x80000000 |
						  system_info.si_dramstart) +
						 MEM_BLOCK_SIZE * m +
						 pagesize * i);
					page->magic0 = 0;
					page->magic1 = 0;
					if (npages <= ++nfounds) {
						break;
					}
				} else {
					debug_printf(TEXT("invalid page header\n"));
					msg_printf(MSG_ERROR, whoami, TEXT("invalid page header\n"));
					goto error_cleanup;
				}
			}
		}
		VirtualFree(mem, 0, MEM_RELEASE);
	}

	if (nfounds < npages) {
		debug_printf(TEXT("lost %d pages\n"), npages - nfounds);
		msg_printf(MSG_ERROR, whoami, 
			   TEXT("lost %d pages (allocated %d pages)\n"), 
			   npages - nfounds, npages);
		goto error_cleanup;
	}

	/*
	 *  allocate root page
	 */
	if ((map = (struct map_s*)vmem_alloc()) == NULL) {
		debug_printf(TEXT("can't allocate root page.\n"));
		msg_printf(MSG_ERROR, whoami, TEXT("can't allocate root page.\n"));
		goto error_cleanup;
	}
	map->nleaves = nleaves;
	map->leafsize = pagesize / sizeof(caddr_t);
	map->pagesize = pagesize;

	/*
	 *  allocate leaf pages
	 */
	for (i = 0; i < nleaves; i++) {
		if ((map->leaf[i] = (caddr_t*)vmem_alloc()) == NULL) {
			debug_printf(TEXT("can't allocate leaf page.\n"));
			msg_printf(MSG_ERROR, whoami, TEXT("can't allocate leaf page.\n"));
			goto error_cleanup;
		}
	}

	/*
	 *  allocate kernel pages
	 */
	for (i = 0; start < kernel_end; start += pagesize, i++) {
		caddr_t *leaf = map->leaf[i / map->leafsize];
		if ((leaf[i % map->leafsize] = alloc_kpage(start)) == NULL) {
			debug_printf(TEXT("can't allocate page 0x%x.\n"), start);
			msg_printf(MSG_ERROR, whoami, TEXT("can't allocate page 0x%x.\n"), start);
			goto error_cleanup;
		}
	}
	map->leaf[i / map->leafsize][i % map->leafsize] = NULL; /* END MARK */

	return (0);

 error_cleanup:
	vmem_free();

	return (-1);
}

void
vmem_free()
{
	map = NULL;
	if (heap) {
		VirtualFree(heap, 0, MEM_RELEASE);
		heap = NULL;
	}
	if (phys_addrs) {
		VirtualFree(phys_addrs, 0, MEM_RELEASE);
		phys_addrs = NULL;
	}
}

void
vmem_dump_map()
{
	caddr_t addr, page, paddr;

	if (map == NULL) {
		debug_printf(TEXT("no page map\n"));
		return;
	}

	for (addr = kernel_start; addr < kernel_end; addr += system_info.si_pagesize) {
		page = vmem_get(addr, NULL);
		paddr = vtophysaddr(page);
		debug_printf(TEXT("%08X: vaddr=%08X paddr=%08X %s\n"),
			     addr, page, paddr, addr == paddr ? TEXT("*") : TEXT("reloc"));
    
	}
}
