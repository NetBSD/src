/*	$NetBSD: xmd.c,v 1.1.2.6 2010/11/04 07:30:00 uebayasi Exp $	*/

/*-
 * Copyright (c) 2010 Tsubai Masanari.  All rights reserved.
 * Copyright (c) 2010 Masao Uebayashi.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xmd.c,v 1.1.2.6 2010/11/04 07:30:00 uebayasi Exp $");

#include "opt_xip.h"
#include "opt_xmd.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/kmem.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/kmem.h>

#include <machine/vmparam.h>

#include <uvm/uvm_extern.h>

struct xmd_softc {
	vaddr_t sc_addr;
	size_t sc_size;

	void *sc_phys;

	struct disk sc_dkdev;
	struct bufq_state *sc_buflist;
};

void xmdattach(int);
static void xmd_attach(device_t, device_t, void *);
static int xmd_detach(device_t, int);
static dev_type_open(xmd_open);
static dev_type_close(xmd_close);
static dev_type_ioctl(xmd_ioctl);
static dev_type_read(xmd_read);
static dev_type_mmap(xmd_mmap);
static dev_type_strategy(xmd_strategy);
static dev_type_size(xmd_size);

struct bdevsw xmd_bdevsw = {
	xmd_open, xmd_close, xmd_strategy, xmd_ioctl,
	nodump, xmd_size, D_DISK | D_MPSAFE
};

struct cdevsw xmd_cdevsw = {
	xmd_open, xmd_close, xmd_read, nowrite, xmd_ioctl,
	nostop, notty, nopoll, xmd_mmap, nokqfilter, D_DISK | D_MPSAFE
};

extern struct cfdriver xmd_cd;
CFATTACH_DECL3_NEW(xmd, sizeof(struct xmd_softc),
    NULL, xmd_attach, xmd_detach, NULL, NULL, NULL, DVF_DETACH_SHUTDOWN);

const char xmd_root_image[XMD_ROOT_SIZE << DEV_BSHIFT] __aligned(PAGE_SIZE) =
    "|This is the root ramdisk!\n";
const size_t xmd_root_size = XMD_ROOT_SIZE << DEV_BSHIFT;

void
xmdattach(int n)
{
	int i;
	cfdata_t cf;

	if (config_cfattach_attach("xmd", &xmd_ca)) {
		aprint_error("xmd: cfattach_attach failed\n");
		return;
	}

	/* XXX Support single instance for now. */
	KASSERT(n == 1);

	for (i = 0; i < n; i++) {
		cf = kmem_alloc(sizeof(*cf), KM_SLEEP);
		KASSERT(cf != NULL);
		cf->cf_name = "xmd";
		cf->cf_atname = "xmd";
		cf->cf_unit = i;
		cf->cf_fstate = FSTATE_NOTFOUND;
		(void)config_attach_pseudo(cf);
	}
}

static void
xmd_attach(device_t parent, device_t self, void *aux)
{
	struct xmd_softc *sc = device_private(self);

	sc->sc_addr = (vaddr_t)xmd_root_image;
	sc->sc_size = (size_t)xmd_root_size;

	sc->sc_phys = pmap_physload_device(sc->sc_addr, sc->sc_size, PROT_READ, 0);

	disk_init(&sc->sc_dkdev, device_xname(self), NULL);
	disk_attach(&sc->sc_dkdev);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
xmd_detach(device_t self, int flags)
{
	struct xmd_softc *sc = device_private(self);
	int rc;

	rc = 0;
	mutex_enter(&sc->sc_dkdev.dk_openlock);
	if (sc->sc_dkdev.dk_openmask == 0)
		;	/* nothing to do */
	else if ((flags & DETACH_FORCE) == 0)
		rc = EBUSY;
	mutex_exit(&sc->sc_dkdev.dk_openlock);

	if (rc != 0)
		return rc;

	pmf_device_deregister(self);
	disk_detach(&sc->sc_dkdev);
	disk_destroy(&sc->sc_dkdev);

	pmap_physunload_device(sc->sc_phys);

	return 0;
}

static int
xmd_open(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct xmd_softc *sc = device_lookup_private(&xmd_cd, DISKUNIT(dev));
	struct disk *dk = &sc->sc_dkdev;
	const int part = DISKUNIT(dev);
	const int pmask = 1 << part;

	if (sc == NULL)
		return ENXIO;

	mutex_enter(&dk->dk_openlock);
	switch (fmt) {
		case S_IFCHR:
		dk->dk_copenmask |= pmask;
		break;
	case S_IFBLK:
		dk->dk_bopenmask |= pmask;
		break;
	}
	dk->dk_openmask = dk->dk_copenmask | dk->dk_bopenmask;
	mutex_exit(&dk->dk_openlock);

	return 0;
}

static int
xmd_close(dev_t dev, int flags, int fmt, struct lwp *l)
{

	return 0;
}

int
xmd_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct xmd_softc *sc = device_lookup_private(&xmd_cd, DISKUNIT(dev));
	const int part = DISKUNIT(dev);
	int error = 0;

	if (sc == NULL)
		return -1;

	switch (cmd) {
	case DIOCGPHYSSEG:
		if (sc->sc_phys == NULL)
			error = EINVAL;
		else
			*(void **)data = sc->sc_phys;
		break;

	case DIOCGDINFO:
		*(struct disklabel *)data = *sc->sc_dkdev.dk_label;
		break;

	case DIOCGPART:
		((struct partinfo *)data)->disklab = sc->sc_dkdev.dk_label;
		((struct partinfo *)data)->part =
		    &sc->sc_dkdev.dk_label->d_partitions[part];
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

static int
xmd_read(dev_t dev, struct uio *uio, int flags)
{
	struct xmd_softc *sc;

	sc = device_lookup_private(&xmd_cd, DISKUNIT(dev));

	if (sc == NULL)
		return ENXIO;

	return physio(xmd_strategy, NULL, dev, B_READ, minphys, uio);
}

paddr_t
xmd_mmap(dev_t dev, off_t off, int prot)
{
	struct xmd_softc *sc = device_lookup_private(&xmd_cd, DISKUNIT(dev));

	if (sc == NULL)
		return -1;

	if ((u_int64_t)off >= sc->sc_size)
		return -1;

	return pmap_mmap(sc->sc_addr, off);
}

static void
xmd_strategy(struct buf *bp)
{
	struct xmd_softc *sc;
	void *addr;
	size_t off, count;

	sc = device_lookup_private(&xmd_cd, DISKUNIT(bp->b_dev));

	off = bp->b_blkno << DEV_BSHIFT;

	/* XXX is b_bcount==0 legal? */

	if (off >= sc->sc_size) {
		if (bp->b_flags & B_READ)
			/* XXX why not error? */
			goto done;
		bp->b_error = EIO;
		goto done;
	}

	if (bp->b_bcount <= (sc->sc_size - off))
		count = bp->b_bcount;
	else
		count = sc->sc_size - off;

	addr = (void *)(sc->sc_addr + off);

	if (bp->b_flags & B_READ)
		memcpy(bp->b_data, addr, count);
	else
		panic("%s: block write is not supported", __func__);

	bp->b_resid = bp->b_bcount - count;

 done:
	biodone(bp);
}

static int
xmd_size(dev_t dev)
{
	struct xmd_softc *sc;

	sc = device_lookup_private(&xmd_cd, DISKUNIT(dev));
	if (sc == NULL)
		return 0;

	return sc->sc_size >> DEV_BSHIFT;
}
