/*	$NetBSD: md_hooks.c,v 1.7.2.1 1997/01/30 05:27:23 thorpej Exp $	*/

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

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <vm/vm.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>

#include <dev/md.h>

#ifndef MEMORY_DISK_SIZE
#define MEMORY_DISK_SIZE 0
#endif

extern u_int memory_disc_size;
struct md_conf *bootmd = NULL;

int md_match_hook __P((struct device *parent, void *self, void *aux));
extern int load_memory_disc_from_floppy __P((struct md_conf *md, dev_t dev));

/*
 * This is called during autoconfig.
 */
int
md_match_hook(parent, self, aux)
	struct device	*parent;
	void	*self;
	void	*aux;
{
	return (1);
}

void
md_attach_hook(unit, md)
	int unit;
	struct md_conf *md;
{
	if (unit == 0) {
		if (memory_disc_size == 0 && MEMORY_DISK_SIZE)
			memory_disc_size = (MEMORY_DISK_SIZE * 1024);

		if (memory_disc_size != 0) {
			md->md_size = round_page(memory_disc_size);
			md->md_addr = (caddr_t)kmem_alloc(kernel_map, memory_disc_size);
			md->md_type = MD_KMEM_FIXED;
			bootmd = md;
		}
	}
	printf("md%d: allocated %dK (%d blocks)\n", unit, md->md_size / 1024, md->md_size / DEV_BSIZE);
}


/*
 * This is called during open (i.e. mountroot)
 */

void
md_open_hook(unit, md)
	int unit;
	struct md_conf *md;
{
/* I use the memory disc for other testing ... */
#if 0
	if (unit == 0) {
		/* The root memory disk only works single-user. */
		boothowto |= RB_SINGLE;
	}
#endif
}
