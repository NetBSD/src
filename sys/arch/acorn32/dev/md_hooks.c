/*	$NetBSD: md_hooks.c,v 1.8.60.1 2009/05/13 17:16:02 jym Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: md_hooks.c,v 1.8.60.1 2009/05/13 17:16:02 jym Exp $");

#include "opt_md.h"

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/md.h>

#ifdef	MEMORY_DISK_ROOT_SIZE
#define ROOTBYTES (MEMORY_DISK_ROOT_SIZE << DEV_BSHIFT)

/*
 * This array will be patched to contain a file-system image.
 * See the program:  src/distrib/sun3/common/rdsetroot.c
 */
size_t md_root_size = ROOTBYTES;
char md_root_image[ROOTBYTES] = "|This is the root ramdisk!\n";

#else	/* MEMORY_DISK_ROOT_SIZE */

size_t md_root_size = 0;		/* set by machdep.c */
static struct md_conf *bootmd = NULL;

extern int load_memory_disc_from_floppy(struct md_conf *md, dev_t dev);

#include "fdc.h"
#endif	/* MEMORY_DISK_ROOT_SIZE */

void
md_attach_hook(int unit, struct md_conf *md)
{
	if (unit == 0) {
#ifdef MEMORY_DISK_ROOT_SIZE
		/* Setup root ramdisk */
		md->md_addr = (void *) md_root_image;
		md->md_size = (size_t)  md_root_size;
		md->md_type = MD_KMEM_FIXED;
#else	/* MEMORY_DISK_ROOT_SIZE */
#ifdef OLD_MEMORY_DISK_SIZE
		if (md_root_size == 0 && OLD_MEMORY_DISK_SIZE)
			md_root_size = (OLD_MEMORY_DISK_SIZE << DEV_BSHIFT);
#endif	/* OLD_MEMORY_DISK_SIZE */
		if (md_root_size != 0) {
			md->md_size = round_page(md_root_size);
			md->md_addr = (void *)uvm_km_alloc(kernel_map,
			    md_root_size, 0, UVM_KMF_WIRED | UVM_KMF_ZERO);
			md->md_type = MD_KMEM_FIXED;
			bootmd = md;
		}
#endif	/* MEMORY_DISK_ROOT_SIZE */
		printf("md%d: allocated %ldK (%ld blocks)\n", unit, (long)md->md_size / 1024, (long)md->md_size / DEV_BSIZE);
	}
}


/*
 * This is called during open (i.e. mountroot)
 */

void
md_open_hook(int unit, struct md_conf *md)
{
	if (unit == 0) {
		/* The root memory disk only works single-user. */
		boothowto |= RB_SINGLE;
#if !defined(MEMORY_DISK_ROOT_SIZE) && NFDC > 0
		load_memory_disc_from_floppy(bootmd, makedev(17, 1));	/* XXX 1.44MB FD */
#endif
	}
}
