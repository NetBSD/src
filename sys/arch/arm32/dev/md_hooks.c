/*	$NetBSD: md_hooks.c,v 1.15 2000/06/26 14:20:35 mrg Exp $	*/

/*
 * Copyright (c) 1995 Gordon W. Ross
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include "opt_md.h"

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <vm/vm.h>

#include <uvm/uvm_extern.h>

#include <dev/md.h>

#include "opt_mdsize.h"

#ifdef	MINIROOTSIZE
#define ROOTBYTES (MINIROOTSIZE << DEV_BSHIFT)

/*
 * This array will be patched to contain a file-system image.
 * See the program:  src/distrib/sun3/common/rdsetroot.c
 */
int md_root_size = ROOTBYTES;
char md_root_image[ROOTBYTES] = "|This is the root ramdisk!\n";

#else	/* MINIROOTSIZE */

u_int memory_disc_size = 0;		/* set by machdep.c */
static struct md_conf *bootmd = NULL;

extern int load_memory_disc_from_floppy __P((struct md_conf *md, dev_t dev));

#include "fdc.h"
#endif	/* MINIROOTSIZE */

void
md_attach_hook(unit, md)
	int unit;
	struct md_conf *md;
{
	if (unit == 0) {
#ifdef MINIROOTSIZE
		/* Setup root ramdisk */
		md->md_addr = (caddr_t) md_root_image;
		md->md_size = (size_t)  md_root_size;
		md->md_type = MD_KMEM_FIXED;
#else	/* MINIROOTSIZE */
#ifdef MEMORY_DISK_SIZE
		if (memory_disc_size == 0 && MEMORY_DISK_SIZE)
			memory_disc_size = (MEMORY_DISK_SIZE << DEV_BSHIFT);
#endif	/* MEMORY_DISK_SIZE */
		if (memory_disc_size != 0) {
			md->md_size = round_page(memory_disc_size);
			md->md_addr = (caddr_t)uvm_km_zalloc(kernel_map, memory_disc_size);
			md->md_type = MD_KMEM_FIXED;
			bootmd = md;
		}
#endif	/* MINIROOTSIZE */
		printf("md%d: allocated %dK (%d blocks)\n", unit, md->md_size / 1024, md->md_size / DEV_BSIZE);
	}
}


/*
 * This is called during open (i.e. mountroot)
 */

void
md_open_hook(unit, md)
	int unit;
	struct md_conf *md;
{
	if (unit == 0) {
		/* The root memory disk only works single-user. */
		boothowto |= RB_SINGLE;
#if !defined(MINIROOTSIZE) && NFDC > 0
		load_memory_disc_from_floppy(bootmd, makedev(17, 1));	/* XXX 1.44MB FD */
#endif
	}
}
