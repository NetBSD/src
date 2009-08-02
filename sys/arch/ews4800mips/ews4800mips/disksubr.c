/*	$NetBSD: disksubr.c,v 1.8 2009/08/02 12:04:28 uch Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.8 2009/08/02 12:04:28 uch Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/disk.h>
#include <sys/buf.h>
#include <sys/disklabel.h>

#include <machine/sector.h>

#define	DISKLABEL_DEBUG

#ifdef DISKLABEL_DEBUG
#define	DPRINTF(fmt, args...)	printf(fmt, ##args)
#else
#define	DPRINTF(arg...)		((void)0)
#endif

const char *
readdisklabel(dev_t dev, void (*strategy)(struct buf *), struct disklabel *d,
    struct cpu_disklabel *ux)
{
	uint8_t buf[DEV_BSIZE];
	struct pdinfo_sector *pdinfo = &ux->pdinfo;
	struct vtoc_sector *vtoc = &ux->vtoc;
	bool disklabel_available = false;
	bool vtoc_available = false;
	void *rwops;

	if ((rwops = sector_init(dev, strategy)) == 0)
		return "can't read/write disk";

	/* Read VTOC */
	if (!pdinfo_sector(rwops, pdinfo) || !pdinfo_sanity(pdinfo)) {
		DPRINTF("%s: PDINFO not found.\n", __func__);
	} else if (vtoc_sector(rwops, vtoc, pdinfo->logical_sector) &&
	    vtoc_sanity(vtoc)) {
		vtoc_available = true;

		/* Read BSD DISKLABEL (if any) */
		sector_read(rwops, buf, LABELSECTOR);
		if (disklabel_sanity((struct disklabel *)buf)) {
			disklabel_available = true;
			memcpy(d, buf, sizeof(struct disklabel));
		} else {
			DPRINTF("%s: no BSD disklabel.\n", __func__);
		}
	} else {
		DPRINTF("%s: PDINFO found, but VTOC not found.\n", __func__);
	}
	sector_fini(rwops);

	/* If there is no BSD disklabel, convert from VTOC */
	if (!disklabel_available) {
		if (vtoc_available) {
			DPRINTF("%s: creating disklabel from VTOC.\n",
			    __func__);
		} else {
			DPRINTF("%s: no VTOC. creating default disklabel.\n",
			    __func__);
			vtoc_set_default(ux, d);
		}
		disklabel_set_default(d);
		vtoc_to_disklabel(ux, d);
	}

	return 0;
}

int
setdisklabel(struct disklabel *od, struct disklabel *nd, u_long openmask,
    struct cpu_disklabel *ux)
{

	KDASSERT(openmask == 0); /* openmask is obsolete. -uch */

	if (!disklabel_sanity(nd))
		return EINVAL;

	*od = *nd;

	return 0;
}

int
writedisklabel(dev_t dev, void (*strategy)(struct buf *), struct disklabel *d,
    struct cpu_disklabel *ux)
{
	uint8_t buf[DEV_BSIZE];
	int err = 0;
	void *rwops;

	if (!disklabel_sanity(d))
		return EINVAL;

	/* 1. Update VTOC */
	disklabel_to_vtoc(ux, d);
	DPRINTF("%s: logical_sector=%d\n", __func__, ux->pdinfo.logical_sector);

	if ((rwops = sector_init(dev, strategy)) == 0)
		return ENOMEM;
	pdinfo_sanity(&ux->pdinfo);
	vtoc_sanity(&ux->vtoc);

	/* 2. Write VTOC to bootblock */
	sector_write(rwops, (void *)&ux->pdinfo, PDINFO_SECTOR);
	sector_write(rwops, (void *)&ux->vtoc,
	    ux->pdinfo.logical_sector + VTOC_SECTOR);

	/* 3. Write disklabel to LABELSECTOR */
	memset(buf, 0, sizeof buf);
	memcpy(buf, d, sizeof *d);
	if (!sector_write(rwops, buf, LABELSECTOR)) {
		DPRINTF("%s: failed to write disklabel.\n", __func__);
		err = EIO;
	}
	sector_fini(rwops);

	return err;
}
