/*	$NetBSD: mapper.c,v 1.4 2020/09/06 02:18:53 riastradh Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mapper.c,v 1.4 2020/09/06 02:18:53 riastradh Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

/*
 * Creating a device /dev/mapper for demonstration.
 * To use this device you need to do:
 * 	mknod /dev/mapper c 351 0
 *
 */

dev_type_open(mapper_open);
dev_type_close(mapper_close);
dev_type_mmap(mapper_mmap);

static struct cdevsw mapper_cdevsw = {
	.d_open = mapper_open,
	.d_close = mapper_close,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = mapper_mmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

struct mapper_softc {
	int refcnt;
	char *buffer;
};

static struct mapper_softc sc;

int
mapper_open(dev_t self __unused, int flag __unused, int mode __unused,
           struct lwp *l __unused)
{
	if (sc.refcnt > 0)
		return EBUSY;
	++sc.refcnt;
	sc.buffer = kmem_zalloc(PAGE_SIZE, KM_SLEEP);
	snprintf(sc.buffer, PAGE_SIZE, "Hey There!");
	return 0;
}

int
mapper_close(dev_t self __unused, int flag __unused, int mode __unused,
            struct lwp *l __unused)
{
	kmem_free(sc.buffer, PAGE_SIZE);
	--sc.refcnt;
	return 0;
}

/*
 * Here, pmap_extract() is used to extract the mapping from the specified
 * physical map for the provided virtual address, where pmap_kernel() points
 * to the kernel pmap.
 */

paddr_t
mapper_mmap(dev_t dev, off_t off, int prot)
{
	paddr_t pa;

	if (off & PAGE_MASK)
		return (paddr_t)-1;
	if (prot != VM_PROT_READ)
		return (paddr_t)-1;
	if (pmap_extract(pmap_kernel(), (vaddr_t)sc.buffer, &pa))
		return (paddr_t)atop(pa);

	return (paddr_t)-1;
}

MODULE(MODULE_CLASS_MISC, mapper, NULL);

static int
mapper_modcmd(modcmd_t cmd, void *arg __unused)
{
	/* The major should be verified and changed if needed to avoid
	 * conflicts with other devices. */
	int cmajor = 351, bmajor = -1;

	switch (cmd) {
	case MODULE_CMD_INIT:
		if (devsw_attach("mapper", NULL, &bmajor, &mapper_cdevsw,
					&cmajor))
			return ENXIO;
		return 0;
	case MODULE_CMD_FINI:
		if (sc.refcnt > 0)
			return EBUSY;
		devsw_detach(NULL, &mapper_cdevsw);
		return 0;
	default:
		return ENOTTY;
	}
	return 0;
}
