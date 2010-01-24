/*	$NetBSD: amiga_init.c,v 1.9 2010/01/24 15:39:50 phx Exp $	*/

/*
 * Copyright (c) 1994 Michael L. Hitch
 * Copyright (c) 1993 Markus Wild
 * All rights reserved.
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
 *      This product includes software developed by Markus Wild.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amiga_init.c,v 1.9 2010/01/24 15:39:50 phx Exp $");

#include <sys/param.h>
#include <amiga/amiga/cc.h>
#include <amiga/amiga/cia.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cfdev.h>
#include <amiga/amiga/memlist.h>

int machineid;

vaddr_t CHIPMEMADDR;
vaddr_t chipmem_start;
vaddr_t chipmem_end;

u_long boot_fphystart, boot_fphysize, cphysize;
u_long boot_flags;

char *esym;			/* start address of cfdev/memlist-area */

struct boot_memlist *memlist;

struct cfdev *cfdev;
int ncfdev;

u_long scsi_nosync;
int shift_nosync;

void set_boot_args(int, u_long, u_long, u_long, char *, int, u_long, u_long);


/*
 * called from locore.S, before initppc()
 */
void
set_boot_args(int boot_howto, u_long fstart, u_long fsize, u_long csize,
              char *esymaddr, int cpuid, u_long flags, u_long inhsync)
{
	char *end_loaded;

	boothowto = boot_howto;
	boot_fphystart = fstart;
	boot_fphysize = fsize;
	cphysize = csize;
	esym = end_loaded = esymaddr;
	machineid = cpuid;
	boot_flags = flags;
	scsi_nosync = inhsync;

	/*
	 * The kernel ends at end_loaded, plus the cfdev and memlist
	 * structures we placed there in the loader.
	 * esym already takes a potential symbol-table into account.
	 */
	ncfdev = *(int *)end_loaded;
	cfdev = (struct cfdev *)(end_loaded + sizeof(int));
	end_loaded += sizeof(int) + ncfdev * sizeof(struct cfdev);
	memlist = (struct boot_memlist *)end_loaded;

	CHIPMEMADDR = 0x0;
	chipmem_start = CHIPMEMADDR;
	chipmem_end  = (vaddr_t)csize;

	CIAADDR = 0xbfd000;
	CIAAbase = CIAADDR + 0x1001;
	CIABbase = CIAADDR;

	CUSTOMADDR = 0xdff000;
	CUSTOMbase = CUSTOMADDR;
}

/*
 * called by cc_init_chipmem() from amiga/amiga/cc.c
 */
void *
chipmem_steal(long amount)
{

	/* steal from top of chipmem, as amiga68k does */
	vaddr_t p = chipmem_end - amount;
	if (p & 1)
		p = p - 1;
	chipmem_end = p;
	if(chipmem_start > chipmem_end)
		panic("not enough chip memory");
	return((void *)p);
}

/*
 * Used by certain drivers currently to allocate zorro II or chip mem
 * for bounce buffers. For amigappc we will only return chip memory.
 */
void *
alloc_z2mem(long amount)
{

	return (alloc_chipmem(amount));
}
