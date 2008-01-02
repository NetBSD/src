/*	$NetBSD: sector.c,v 1.5.8.1 2008/01/02 21:47:42 bouyer Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sector.c,v 1.5.8.1 2008/01/02 21:47:42 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <machine/sector.h>

struct sector_rw {
	struct buf *buf;
	void (*strategy)(struct buf *);
	bool busy;
} __context;

void *
sector_init(dev_t dev, void (*strat)(struct buf *))
{
	struct sector_rw *rw =& __context;

	if (rw->busy)
		return 0;
	rw->busy = true;
	rw->strategy = strat;
	rw->buf = geteblk(DEV_BSIZE);
	rw->buf->b_dev = dev;

	return rw;
}

void
sector_fini(void *self)
{
	struct sector_rw *rw = self;

	brelse(rw->buf, 0);
	rw->busy = false;
}

bool
sector_read_n(void *self, uint8_t *buf, daddr_t sector, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		if (!sector_read(self, buf, sector))
			return false;
		buf += DEV_BSIZE;
		sector++;
	}

	return true;
}

bool
sector_read(void *self, uint8_t *buf, daddr_t sector)
{
	struct sector_rw *rw = self;
	struct buf *b = rw->buf;

	b->b_blkno = sector;
	b->b_cylinder = sector / 100;
	b->b_bcount = DEV_BSIZE;
	b->b_oflags &= ~(BO_DONE);
	b->b_flags |= B_READ;
	rw->strategy(b);

	if (biowait(b) != 0)
		return false;

	memcpy(buf, b->b_data, DEV_BSIZE);

	return true;
}

bool
sector_write_n(void *self, uint8_t *buf, daddr_t sector, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		if (!sector_write(self, buf, sector))
			return false;
		buf += DEV_BSIZE;
		sector++;
	}

	return true;
}

bool
sector_write(void *self, uint8_t *buf, daddr_t sector)
{
	struct sector_rw *rw = self;
	struct buf *b = rw->buf;

	b->b_blkno = sector;
	b->b_cylinder = sector / 100;
	b->b_bcount = DEV_BSIZE;
	b->b_flags &= ~(B_READ);
	b->b_oflags &= ~(BO_DONE);
	b->b_flags |= B_WRITE;
	memcpy(b->b_data, buf, DEV_BSIZE);
	rw->strategy(b);

	if (biowait(b) != 0)
		return false;

	return true;
}
