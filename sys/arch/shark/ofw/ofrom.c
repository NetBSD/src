/*	$NetBSD: ofrom.c,v 1.25.2.1 2014/08/10 06:54:08 tls Exp $	*/

/*
 * Copyright 1998
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * XXX open for writing (for user programs to mmap, and write contents)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofrom.c,v 1.25.2.1 2014/08/10 06:54:08 tls Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <dev/ofw/openfirm.h>

struct ofrom_softc {
	int		enabled;
	paddr_t		base;
	paddr_t		size;
};

int ofromprobe(device_t, cfdata_t, void *);
void ofromattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ofrom, sizeof(struct ofrom_softc),
    ofromprobe, ofromattach, NULL, NULL);

extern struct cfdriver ofrom_cd;

dev_type_open(ofromopen);
dev_type_read(ofromrw);
dev_type_mmap(ofrommmap);

const struct cdevsw ofrom_cdevsw = {
	.d_open = ofromopen,
	.d_close = nullclose,
	.d_read = ofromrw,
	.d_write = ofromrw,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = ofrommmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

int
ofromprobe(device_t parent, cfdata_t cf, void *aux)
{
	struct ofbus_attach_args *oba = aux;
	static const char *const compatible_strings[] = { "rom", NULL };

	return (of_compatible(oba->oba_phandle, compatible_strings) == -1) ?
	    0 : 5;
}


void
ofromattach(device_t parent, device_t self, void *aux)
{
	struct ofrom_softc *sc = device_private(self);
	struct ofbus_attach_args *oba = aux;
	char regbuf[8];

	if (OF_getproplen(oba->oba_phandle, "reg") != 8) {
		printf(": invalid reg property\n");
		return;
	}
	if (OF_getprop(oba->oba_phandle, "reg", regbuf, sizeof regbuf) != 8) {
		printf(": couldn't read reg property\n");
		return;
	}
	sc->base = of_decode_int(&regbuf[0]);
	sc->size = of_decode_int(&regbuf[4]);
	sc->enabled = 1;

	printf(": %#lx-%#lx\n", sc->base, sc->base + sc->size - 1);
}

int
ofromopen(dev_t dev, int oflags, int devtype, struct lwp *l)
{
	struct ofrom_softc *sc;

	sc = device_lookup_private(&ofrom_cd, minor(dev));
	if (!sc || !sc->enabled)
		return (ENXIO);

	if (oflags & FWRITE)
		return (EINVAL);

	return (0);
}

int
ofromrw(dev_t dev, struct uio *uio, int flags)
{
	pmap_t kpm = pmap_kernel();
	struct ofrom_softc *sc;
	int c, error = 0;
	struct iovec *iov;
	paddr_t v;
	psize_t o;
	extern kmutex_t memlock;
	extern char *memhook;

	sc = device_lookup_private(&ofrom_cd, minor(dev));
	if (!sc || !sc->enabled)
		return (ENXIO);			/* XXX PANIC */

	mutex_enter(&memlock);
	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("ofromrw");
			continue;
		}

		/*
		 * Since everything is page aligned and no more
		 * than the rest of a page is done at once, we
		 * can just check that the offset isn't too big.
		 */
		if (uio->uio_offset >= sc->size)
			break;

		/* XXX: Use unamanged mapping. */
		v = sc->base + uio->uio_offset;
		pmap_kenter_pa((vaddr_t)memhook, trunc_page(v),
		    uio->uio_rw == UIO_READ ?  VM_PROT_READ : VM_PROT_WRITE,
		    0);
		pmap_update(kpm);
		o = uio->uio_offset & PGOFSET;
		c = min(uio->uio_resid, (int)(PAGE_SIZE - o));
		error = uiomove((char *)memhook + o, c, uio);
		pmap_kremove((vaddr_t)memhook, (vaddr_t)memhook + PAGE_SIZE);
		pmap_update(kpm);
	}
	mutex_exit(&memlock);

	return (error);
}

paddr_t
ofrommmap(dev_t dev, off_t off, int prot)
{
	struct ofrom_softc *sc;

	sc = device_lookup_private(&ofrom_cd, minor(dev));
	if (!sc || !sc->enabled)
		return (-1);			/* XXX PANIC */

	if ((u_int)off >= sc->size)
		return (-1);

	return arm_btop(sc->base + off);
}
