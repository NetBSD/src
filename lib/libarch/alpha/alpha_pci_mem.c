/*	$NetBSD: alpha_pci_mem.c,v 1.2 2000/06/30 18:19:28 simonb Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Support for mapping PCI/EISA/ISA memory space.  This is currently used
 * to provide such support for XFree86.  In a perfect world, this would go
 * away in favor of a real bus space mapping framework.
 */

#include <sys/param.h>
#include <sys/mman.h>

#include <machine/sysarch.h>

#include <fcntl.h>
#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct alpha_bus_window *alpha_pci_mem_windows;
int alpha_pci_mem_window_count;

void *
alpha_pci_mem_map(memaddr, memsize, flags, rabst)
	bus_addr_t memaddr;
	bus_size_t memsize;
	int flags;
	struct alpha_bus_space_translation *rabst;
{
	struct alpha_bus_window *abw;
	void *addr;
	int prefetchable = flags & BUS_SPACE_MAP_PREFETCHABLE;
	int linear = flags & BUS_SPACE_MAP_LINEAR;
	bus_size_t offset;
	int i, fd;

	/*
	 * Can't have linear without prefetchable.
	 */
	if (linear && !prefetchable)
		return (MAP_FAILED);

	if (alpha_pci_mem_windows == NULL) {
		i = alpha_bus_getwindows(ALPHA_BUS_TYPE_PCI_MEM, &abw);
		if (i <= 0)
			return (MAP_FAILED);

		alpha_pci_mem_windows = abw;
		alpha_pci_mem_window_count = i;
	}

	for (i = 0; i < alpha_pci_mem_window_count; i++) {
		abw = &alpha_pci_mem_windows[i];
		if (memaddr < abw->abw_abst.abst_bus_start ||
		    (memaddr + (memsize - 1)) > abw->abw_abst.abst_bus_end)
			continue;

		/* If we want linear, the window must be dense. */
		if (linear && (abw->abw_abst.abst_flags & ABST_DENSE) == 0)
			continue;

		/* Looks like we have a winner! */
		goto found;
	}

 found:
	fd = open(_PATH_MEM, O_RDWR, 0600);
	if (fd == -1)
		return (MAP_FAILED);

	memsize <<= abw->abw_abst.abst_addr_shift;
	offset = (memaddr - abw->abw_abst.abst_bus_start) <<
	    abw->abw_abst.abst_addr_shift;

	addr = mmap(NULL, memsize, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED,
	    fd, (off_t) (abw->abw_abst.abst_sys_start + offset));
	
	(void) close(fd);

	if (addr != MAP_FAILED) {
		/*
		 * Make a copy of the space translation so that the caller
		 * can do the correct access.
		 */
		*rabst = abw->abw_abst;
	}
	return (addr);
}

void
alpha_pci_mem_unmap(abst, addr, size)
	struct alpha_bus_space_translation *abst;
	void *addr;
	bus_size_t size;
{

	(void) munmap(addr, size << abst->abst_addr_shift);
}
