/*	$NetBSD: ofrom.c,v 1.1.2.4 2002/06/20 03:40:58 nathanw Exp $	*/

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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/fcntl.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <dev/ofw/openfirm.h>

struct ofrom_softc {
	struct device	sc_dev;
	int		enabled;
	vm_offset_t	base;
	vm_size_t	size;
};

int ofromprobe __P((struct device *, struct cfdata *, void *));
void ofromattach __P((struct device *, struct device *, void *));

struct cfattach ofrom_ca = {
	sizeof(struct ofrom_softc), ofromprobe, ofromattach
};

extern struct cfdriver ofrom_cd;

cdev_decl(ofrom);

int
ofromprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ofbus_attach_args *oba = aux;
	static const char *const compatible_strings[] = { "rom", NULL };

	return (of_compatible(oba->oba_phandle, compatible_strings) == -1) ?
	    0 : 5;
}


void
ofromattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ofrom_softc *sc = (struct ofrom_softc *)self;
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
ofromopen(dev, oflags, devtype, p)
	dev_t dev;
	int oflags, devtype;
	struct proc *p;
{
	struct ofrom_softc *sc;
	int unit = minor(dev);

	if (unit >= ofrom_cd.cd_ndevs)
		return (ENXIO);
	sc = ofrom_cd.cd_devs[unit];
	if (!sc || !sc->enabled)
		return (ENXIO);

	if (oflags & FWRITE)
		return (EINVAL);

	return (0);
}

int
ofromclose(dev, fflag, devtype, p)
	dev_t dev;
	int fflag, devtype;
	struct proc *p;
{

	return (0);
}

int
ofromrw(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct ofrom_softc *sc;
	int c, error = 0, unit = minor(dev);
	struct iovec *iov;
	vm_offset_t o, v;
	extern int physlock;
	extern char *memhook;

	if (unit >= ofrom_cd.cd_ndevs)
		return (ENXIO);			/* XXX PANIC */
	sc = ofrom_cd.cd_devs[unit];
	if (!sc || !sc->enabled)
		return (ENXIO);			/* XXX PANIC */

	/* lock against other uses of shared vmmap */
	while (physlock > 0) {
		physlock++;
		error = tsleep((caddr_t)&physlock, PZERO | PCATCH, "ofromrw",
		    0);
		if (error)
			return (error);
	}
	physlock = 1;

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

		v = sc->base + uio->uio_offset;
		pmap_enter(pmap_kernel(), (vm_offset_t)memhook,
		    trunc_page(v), uio->uio_rw == UIO_READ ?
		    VM_PROT_READ : VM_PROT_WRITE, PMAP_WIRED);
		pmap_update(pmap_kernel());
		o = uio->uio_offset & PGOFSET;
		c = min(uio->uio_resid, (int)(NBPG - o));
		error = uiomove((caddr_t)memhook + o, c, uio);
		pmap_remove(pmap_kernel(), (vm_offset_t)memhook,
		    (vm_offset_t)memhook + NBPG);
		pmap_update(pmap_kernel());
	}

	if (physlock > 1)
		wakeup((caddr_t)&physlock);
	physlock = 0;

	return (error);
}

int
ofromioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{

	return (ENOTTY);
}

paddr_t
ofrommmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	struct ofrom_softc *sc;
	int unit = minor(dev);

	if (unit >= ofrom_cd.cd_ndevs)
		return (-1);			/* XXX PANIC */
	sc = ofrom_cd.cd_devs[unit];
	if (!sc || !sc->enabled)
		return (-1);			/* XXX PANIC */

	if ((u_int)off >= sc->size)
		return (-1);

	return arm_btop(sc->base + off);
}
