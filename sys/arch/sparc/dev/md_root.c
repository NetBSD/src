/*	$NetBSD: md_root.c,v 1.5.10.1 1997/10/14 10:18:31 thorpej Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/stat.h>

#include <dev/md.h>

extern int boothowto;

int	(*md_read_image) __P((size_t *, caddr_t *));

/*
 * This is called during autoconfig.
 */
void
md_attach_hook(unit, md)
	int unit;
	struct md_conf *md;
{
	if (unit == 0 && md_read_image != NULL) {
		/* Setup root ramdisk */
		if ((*md_read_image)(&md->md_size, &md->md_addr) != 0)
			panic("md_attach");
		md->md_type = MD_KMEM_FIXED;
		printf("md0: fixed, %d blocks", md->md_size >> DEV_BSHIFT);
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
		/* The root ramdisk only works single-user. */
		boothowto |= RB_SINGLE;
	}
}

#if 0
int
md_read_image (dev, addr)
	dev_t dev;
	caddr_t addr;
{
	off_t offset;
	int error;
	extern struct proc proc0;
	struct buf fishtank;
	struct buf *bp = &fishtank;

	error = (*bdevsw[major(dev)].d_open)(dev, 0, S_IFCHR, &proc0);
	if (error)
		panic("md_read_image: open: error %d", error);

	bzero(bp, sizeof(*bp));
	offset = 0;

	for (;;) {
		iovec.iov_base = addr;
		iovec.iov_len = DEV_BSIZE;

		uio.uio_iov = &iovec;
		uio.uio_iovcnt = 1;
		uio.uio_offset = offset;
		uio.uio_resid = DEV_BSIZE;
		uio.uio_segflg = UIO_SYSSPACE;
		uio.uio_rw = UIO_READ;
		uio.uio_procp = &proc0;
		error = (*cdevsw[major(dev)].d_read)(dev, &uio, 0);
		if (error)
			panic("md_read_image: read: error %d", error);

		if (uio.uio_resid != 0)
			break;

		addr += DEV_BSIZE;
		offset += DEV_BSIZE;
		md_root_size += DEV_BSIZE;
		if (offset + DEV_BSIZE > md_root_size)
			break;
	}
	(void)(*bdevsw[major(dev)].d_close)(dev, 0, S_IFCHR, &proc0);
	md_root_size = offset;
	return 0;
}
#endif
