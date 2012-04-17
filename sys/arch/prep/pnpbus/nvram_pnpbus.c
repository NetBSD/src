/* $NetBSD: nvram_pnpbus.c,v 1.15.2.1 2012/04/17 00:06:49 yamt Exp $ */

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
__KERNEL_RCSID(0, "$NetBSD: nvram_pnpbus.c,v 1.15.2.1 2012/04/17 00:06:49 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/kthread.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/simplelock.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <machine/isa_machdep.h>
/* clock stuff for motorolla machines */
#include <dev/clock_subr.h>
#include <dev/ic/mk48txxreg.h>

#include <uvm/uvm_extern.h>

#include <machine/residual.h>
#include <machine/nvram.h>

#include <prep/pnpbus/pnpbusvar.h>

#include "opt_nvram.h"

static char *nvramData;
static NVRAM_MAP *nvram;
static char *nvramGEAp;		/* pointer to the GE area */
static char *nvramCAp;		/* pointer to the Config area */
static char *nvramOSAp;		/* pointer to the OSArea */

int prep_clock_mk48txx;

extern char bootpath[256];
extern RESIDUAL resdata;

#define NVRAM_STD_DEV 0

static int	nvram_pnpbus_probe(device_t, cfdata_t, void *);
static void	nvram_pnpbus_attach(device_t, device_t, void *);
uint8_t		prep_nvram_read_val(int);
char		*prep_nvram_next_var(char *);
char		*prep_nvram_find_var(const char *);
char		*prep_nvram_get_var(const char *);
int		prep_nvram_get_var_len(const char *);
int		prep_nvram_count_vars(void);
void		prep_nvram_write_val(int, uint8_t);
uint8_t		mkclock_pnpbus_nvrd(struct mk48txx_softc *, int);
void		mkclock_pnpbus_nvwr(struct mk48txx_softc *, int, uint8_t);

CFATTACH_DECL(nvram_pnpbus, sizeof(struct nvram_pnpbus_softc),
    nvram_pnpbus_probe, nvram_pnpbus_attach, NULL, NULL);

dev_type_open(prep_nvramopen);
dev_type_ioctl(prep_nvramioctl);
dev_type_close(prep_nvramclose);
dev_type_read(prep_nvramread);

const struct cdevsw nvram_cdevsw = {
	prep_nvramopen, prep_nvramclose, prep_nvramread, nowrite,
	prep_nvramioctl, nostop, notty, nopoll, nommap, nokqfilter, D_OTHER,
};

extern struct cfdriver nvram_cd;

static int
nvram_pnpbus_probe(device_t parent, cfdata_t match, void *aux)
{
	struct pnpbus_dev_attach_args *pna = aux;
	int ret = 0;

	if (strcmp(pna->pna_devid, "IBM0008") == 0)
		ret = 1;

	if (ret)
		pnpbus_scan(pna, pna->pna_ppc_dev);

	return ret;
}

static void
nvram_pnpbus_attach(device_t parent, device_t self, void *aux)
{
	struct nvram_pnpbus_softc *sc = device_private(self);
	struct pnpbus_dev_attach_args *pna = aux;
	int as_iobase, as_len, data_iobase, data_len, i, nvlen, cur;
	uint8_t *p;
	HEADER prep_nvram_header;

	sc->sc_iot = pna->pna_iot;

	pnpbus_getioport(&pna->pna_res, 0, &as_iobase, &as_len);
	pnpbus_getioport(&pna->pna_res, 1, &data_iobase, &data_len);

	if (pnpbus_io_map(&pna->pna_res, 0, &sc->sc_as, &sc->sc_ash) ||
	    pnpbus_io_map(&pna->pna_res, 1, &sc->sc_data, &sc->sc_datah)) {
		aprint_error("nvram: couldn't map registers\n");
		return;
	}

	/* Initialize the nvram header */
	p = (uint8_t *) &prep_nvram_header;
	for (i = 0; i < sizeof(HEADER); i++) 
		*p++ = prep_nvram_read_val(i);

	/*
	 * now that we have the header, we know how big the NVRAM part on
	 * this machine really is.  Malloc space to save a copy.
	 */

	nvlen = 1024 * prep_nvram_header.Size;
	nvramData = malloc(nvlen, M_DEVBUF, M_NOWAIT);
	p = (uint8_t *) nvramData;

	/*
	 * now read the whole nvram in, one chunk at a time, marking down
	 * the main start points as we go.
	 */
	for (i = 0; i < sizeof(HEADER) && i < nvlen; i++)
		*p++ = prep_nvram_read_val(i);
	nvramGEAp = p;
	cur = i;
	for (; i < cur + prep_nvram_header.GELength && i < nvlen; i++)
		*p++ = prep_nvram_read_val(i);
	nvramOSAp = p;
	cur = i;
	for (; i < cur + prep_nvram_header.OSAreaLength && i < nvlen; i++)
		*p++ = prep_nvram_read_val(i);
	nvramCAp = p;
	cur = i;
	for (; i < cur + prep_nvram_header.ConfigLength && i < nvlen; i++)
		*p++ = prep_nvram_read_val(i);

	/* we should be done here.  umm.. yay? */
	nvram = (NVRAM_MAP *)&nvramData[0];
	aprint_normal("\n");
	aprint_verbose("%s: Read %d bytes from nvram of size %d\n",
	    device_xname(self), i, nvlen);

#if defined(NVRAM_DUMP)
	printf("Boot device: %s\n", prep_nvram_get_var("fw-boot-device"));
	printf("Dumping nvram\n");
	for (cur=0; cur < i; cur++) {
		printf("%c", nvramData[cur]);
		if (cur % 70 == 0)
			printf("\n");
	}
#endif
	strncpy(bootpath, prep_nvram_get_var("fw-boot-device"), 256);


	if (prep_clock_mk48txx == 0)
		return;
	/* otherwise, we have a motorolla clock chip.  Set it up. */
	sc->sc_mksc.sc_model = "mk48t18";
	sc->sc_mksc.sc_year0 = 1900;
	sc->sc_mksc.sc_nvrd = mkclock_pnpbus_nvrd;
	sc->sc_mksc.sc_nvwr = mkclock_pnpbus_nvwr;
	/* copy down the bus space tags */
	sc->sc_mksc.sc_bst = sc->sc_as;
	sc->sc_mksc.sc_bsh = sc->sc_ash;
	sc->sc_mksc.sc_data = sc->sc_data;
	sc->sc_mksc.sc_datah = sc->sc_datah;

	aprint_normal("%s: attaching clock", device_xname(self));
	mk48txx_attach((struct mk48txx_softc *)&sc->sc_mksc);
	aprint_normal("\n");
}

/*
 * This function should be called at a high spl only, as it interfaces with
 * real hardware.
 */

uint8_t
prep_nvram_read_val(int addr)
{
	struct nvram_pnpbus_softc *sc;

        sc = device_lookup_private(&nvram_cd, NVRAM_STD_DEV);
	if (sc == NULL)
		return 0;

	/* tell the NVRAM what we want */
	bus_space_write_1(sc->sc_as, sc->sc_ash, 0, addr);
	bus_space_write_1(sc->sc_as, sc->sc_ash, 1, addr>>8);

	return bus_space_read_1(sc->sc_data, sc->sc_datah, 0);
}

/*
 * This function should be called at a high spl only, as it interfaces with
 * real hardware.
 */

void
prep_nvram_write_val(int addr, uint8_t val)
{
	struct nvram_pnpbus_softc *sc;

        sc = device_lookup_private(&nvram_cd, NVRAM_STD_DEV);
	if (sc == NULL)
		return;

	/* tell the NVRAM what we want */
	bus_space_write_1(sc->sc_as, sc->sc_ash, 0, addr);
	bus_space_write_1(sc->sc_as, sc->sc_ash, 1, addr>>8);

	bus_space_write_1(sc->sc_data, sc->sc_datah, 0, val);
}

/* the rest of these should all be called with the lock held */

char *
prep_nvram_next_var(char *name)
{
	char *cp;

	if (name == NULL)
		return NULL;

	cp = name;
	/* skip forward to the first null char */
	while ((cp - nvramGEAp) < nvram->Header.GELength && (*cp != '\0'))
		cp++;
	/* skip nulls */
	while ((cp - nvramGEAp) < nvram->Header.GELength && (*cp == '\0'))
		cp++;
	if ((cp - nvramGEAp) < nvram->Header.GELength)
		return cp;
	else
		return NULL;
}

char *
prep_nvram_find_var(const char *name)
{
	char *cp = nvramGEAp;
	size_t len;

	len = strlen(name);
	while (cp != NULL) {
		if ((strncmp(name, cp, len) == 0) && (cp[len] == '='))
			return cp;
		cp = prep_nvram_next_var(cp);
	}
	return NULL;
}

char *
prep_nvram_get_var(const char *name)
{
	char *cp = nvramGEAp;
	size_t len;

	if (name == NULL)
		return NULL;
	len = strlen(name);
	while (cp != NULL) {
		if ((strncmp(name, cp, len) == 0) && (cp[len] == '='))
			return cp+len+1;
		cp = prep_nvram_next_var(cp);
	}
	return NULL;
}

int
prep_nvram_get_var_len(const char *name)
{
	char *cp = nvramGEAp;
	char *ep;
	size_t len;

	if (name == NULL)
		return -1;

	len = strlen(name);
	while (cp != NULL) {
		if ((strncmp(name, cp, len) == 0) && (cp[len] == '='))
			goto out;
		cp = prep_nvram_next_var(cp);
	}
	return -1;

out:
	ep = cp;
	while (ep != NULL && *ep != '\0')
		ep++;
	return ep-cp;
}

int
prep_nvram_count_vars(void)
{
	char *cp = nvramGEAp;
	int i=0;

	while (cp != NULL) {
		i++;
		cp = prep_nvram_next_var(cp);
	}
	return i;
}

static int
nvramgetstr(int len, char *user, char **cpp)
{
	int error;
	char *cp;

	/* Reject obvious bogus requests */
	if ((u_int)len > (8 * 1024) - 1)
		return ENAMETOOLONG;

	*cpp = cp = malloc(len + 1, M_TEMP, M_WAITOK);
	error = copyin(user, cp, len);
	cp[len] = '\0';
	return error;
}

int
prep_nvramioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	int len, error;
	struct pnviocdesc *pnv;
	char *np, *cp, *name;

	pnv = (struct pnviocdesc *)data;
	error = 0;
	cp = name = NULL;

	switch (cmd) {
	case PNVIOCGET:
		if (pnv->pnv_name == NULL)
			return EINVAL;

		error = nvramgetstr(pnv->pnv_namelen, pnv->pnv_name, &name);
		np = prep_nvram_get_var(name);
		if (np == NULL)
			return EINVAL;
		len = prep_nvram_get_var_len(name);

		if (len > pnv->pnv_buflen) {
			error = ENOMEM;
			break;
		}
		if (len <= 0)
			break;
		error = copyout(np, pnv->pnv_buf, len);
		pnv->pnv_buflen = len;
		break;

	case PNVIOCGETNEXTNAME:
		/* if the first one is null, we give them the first name */
		if (pnv->pnv_name == NULL) {
			cp = nvramGEAp;
		} else {
			error = nvramgetstr(pnv->pnv_namelen, pnv->pnv_name,
			    &name);
			if (!error) {
				np = prep_nvram_find_var(name);
				cp = prep_nvram_next_var(np);
			}
		}
		if (cp == NULL)
			error = EINVAL;
		if (error)
			break;

		np = cp;
		while (*np != '=')
			np++;
		len = np-cp;
		if (len > pnv->pnv_buflen) {
			error = ENOMEM;
			break;
		}
		error = copyout(cp, pnv->pnv_buf, len);
		if (error)
			break;
		pnv->pnv_buflen = len;
		break;

	case PNVIOCGETNUMGE:
		/* count the GE variables */
		pnv->pnv_num = prep_nvram_count_vars();
		break;
	case PNVIOCSET:
		/* this will require some real work.  Not ready yet */
		return ENOTSUP;
	
	default:
		return ENOTTY;
	}
	if (name)
		free(name, M_TEMP);
	return error;
}

int
prep_nvramread(dev_t dev, struct uio *uio, int flags)
{
	int size, resid, error;
	u_int c;
	char *rdata;

	error = 0;
	rdata = (char *)&resdata;

	if (uio->uio_rw == UIO_WRITE) {
		uio->uio_resid = 0;
		return 0;
	}

	switch (minor(dev)) {
	case DEV_NVRAM:
		size = nvram->Header.Size * 1024;
		break;
	case DEV_RESIDUAL:
		size = res->ResidualLength;
		break;
	default:
		return ENXIO;
	}
	resid = size;
	if (uio->uio_resid < resid)
		resid = uio->uio_resid;
	while (resid > 0 && error == 0 && uio->uio_offset < size) {
		switch (minor(dev)) {
		case DEV_NVRAM:
			c = min(resid, PAGE_SIZE);
			error = uiomove(&nvramData[uio->uio_offset], c, uio);
			break;
		case DEV_RESIDUAL:
			c = min(resid, PAGE_SIZE);
			error = uiomove(&rdata[uio->uio_offset], c, uio);
			break;
		default:
			return ENXIO;
		}
	}
	return error;
}

int
prep_nvramopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct nvram_pnpbus_softc *sc;

	sc = device_lookup_private(&nvram_cd, NVRAM_STD_DEV);
	if (sc == NULL)
		return ENODEV;

	if (sc->sc_open)
		return EBUSY;

	sc->sc_open = 1;

	return 0;
}

int
prep_nvramclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct nvram_pnpbus_softc *sc;

	sc = device_lookup_private(&nvram_cd, NVRAM_STD_DEV);
	if (sc == NULL) 
		return ENODEV;
	sc->sc_open = 0;
	return 0;
}

/* Motorola mk48txx clock routines */
uint8_t
mkclock_pnpbus_nvrd(struct mk48txx_softc *osc, int off)
{
	struct prep_mk48txx_softc *sc = (struct prep_mk48txx_softc *)osc;
	uint8_t datum;
	int s;

#ifdef DEBUG
	aprint_debug("mkclock_pnpbus_nvrd(%d)", off);
#endif
	s = splclock();
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, 0, off & 0xff);
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, 1, off >> 8);
	datum = bus_space_read_1(sc->sc_data, sc->sc_datah, 0);
	splx(s);
#ifdef DEBUG
	aprint_debug(" -> %02x\n", datum);
#endif
	return datum;
}

void
mkclock_pnpbus_nvwr(struct mk48txx_softc *osc, int off, uint8_t datum)
{
	struct prep_mk48txx_softc *sc = (struct prep_mk48txx_softc *)osc;
	int s;

#ifdef DEBUG
	aprint_debug("mkclock_isa_nvwr(%d, %02x)\n", off, datum);
#endif
	s = splclock();
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, 0, off & 0xff);
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, 1, off >> 8);
	bus_space_write_1(sc->sc_data, sc->sc_datah, 0, datum);
	splx(s);
}
