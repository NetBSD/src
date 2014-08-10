/* $NetBSD: athflash.c,v 1.7.2.1 2014/08/10 06:54:02 tls Exp $ */

/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * Portions of this code were written by Garrett D'Amore for the
 * Champaign-Urbana Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Naoto Shimazaki of YOKOGAWA Electric Corporation.
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

/*
 * Flash Memory Driver
 *
 * XXX This primitive flash driver does *not* support boot sectored devices,
 * XXX and only supports a fairly limited set of devices, that we are likely to
 * XXX to find in an AP30.
 * XXX
 * XXX We also are only supporting flash widths of 16 _for the moment_, and
 * XXX we are only supporting flash devices that use the AMD command sets.
 * XXX All this should be reviewed and improved to be much more generic.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: athflash.c,v 1.7.2.1 2014/08/10 06:54:02 tls Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <sys/bus.h>

#include <mips/atheros/include/arbusvar.h>

#ifdef FLASH_DEBUG
int	flash_debug = 0;
#define DPRINTF(x)	if (flash_debug) printf x
#else
#define DPRINTF(x)
#endif

struct flash_softc {
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	size_t			sc_size;
	size_t			sc_sector_size;
	int			sc_status;
	u_int8_t		*sc_buf;
};

#define	FLASH_ST_BUSY	0x1

static int flash_probe(device_t, cfdata_t, void *);
static void flash_attach(device_t, device_t, void *);

static int is_block_same(struct flash_softc *, bus_size_t, const void *);
static int toggle_bit_wait(struct flash_softc *, bus_size_t, int, int, int);

static int flash_sector_erase(struct flash_softc *, bus_size_t);
static int flash_sector_write(struct flash_softc *, bus_size_t);

extern struct cfdriver athflash_cd;

CFATTACH_DECL_NEW(athflash, sizeof(struct flash_softc),
	      flash_probe, flash_attach, NULL, NULL);

dev_type_open(flashopen);
dev_type_close(flashclose);
dev_type_read(flashread);
dev_type_write(flashwrite);

const struct cdevsw athflash_cdevsw = {
	.d_open = flashopen,
	.d_close = flashclose,
	.d_read = flashread,
	.d_write = flashwrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

static struct {
	uint16_t		vendor_id;
	uint16_t		device_id;
	const char		*name;
	int			sector_size;
	int			flash_size;
} flash_ids[] = {
	{ 0x00bf, 0x2780, "SST 39VF400", 0x01000, 0x080000 },	/* 512KB */
	{ 0x00bf, 0x2782, "SST 39VF160", 0x01000, 0x200000 },	/* 2MB */
	{ 0xffff, 0xffff, NULL, 0, 0 }				/* end list */
};

static int
flash_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct arbus_attach_args	*aa = aux;
	bus_space_handle_t		ioh;
	int				rv = 0, i;
	uint16_t			venid, devid;

	if (strcmp(aa->aa_name, cf->cf_name) != 0)
		return 0;

	DPRINTF(("trying to map address %x\n", (unsigned)aa->aa_addr));
	if (bus_space_map(aa->aa_bst, aa->aa_addr, aa->aa_size, 0, &ioh))
		return 0;

	/* issue JEDEC query */
	DPRINTF(("issuing JEDEC query\n"));
	bus_space_write_2(aa->aa_bst, ioh, (0x5555 << 1), 0xAAAA);
	bus_space_write_2(aa->aa_bst, ioh, (0x2AAA << 1), 0x5555);
	bus_space_write_2(aa->aa_bst, ioh, (0x5555 << 1), 0x9090);

	delay(100);
	venid = bus_space_read_2(aa->aa_bst, ioh, 0);
	devid = bus_space_read_2(aa->aa_bst, ioh, 2);

	/* issue software exit */
	bus_space_write_2(aa->aa_bst, ioh, 0x0, 0xF0F0);

	for (i = 0; flash_ids[i].name != NULL; i++) {
		if ((venid == flash_ids[i].vendor_id) &&
		    (devid == flash_ids[i].device_id)) {
			rv = 1;
			break;
		}
	}

	bus_space_unmap(aa->aa_bst, ioh, aa->aa_size);
	return rv;
}

static void
flash_attach(device_t parent, device_t self, void *aux)
{
	char nbuf[32];
	struct flash_softc		*sc = device_private(self);
	struct arbus_attach_args	*aa = aux;
	int				i;
	bus_space_tag_t			iot = aa->aa_bst;
	bus_space_handle_t		ioh;
	uint16_t			venid, devid;

	if (bus_space_map(iot, aa->aa_addr, aa->aa_size, 0, &ioh)) {
		printf(": can't map i/o space\n");
                return;
  	}
	
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_status = 0;

	/* issue JEDEC query */
	bus_space_write_2(aa->aa_bst, ioh, (0x5555 << 1), 0xAAAA);
	bus_space_write_2(aa->aa_bst, ioh, (0x2AAA << 1), 0x5555);
	bus_space_write_2(aa->aa_bst, ioh, (0x5555 << 1), 0x9090);

	delay(100);
	venid = bus_space_read_2(aa->aa_bst, ioh, 0);
	devid = bus_space_read_2(aa->aa_bst, ioh, 2);

	/* issue software exit */
	bus_space_write_2(aa->aa_bst, ioh, 0x0, 0xF0F0);

	for (i = 0; flash_ids[i].name != NULL; i++) {
		if ((venid == flash_ids[i].vendor_id) &&
		    (devid == flash_ids[i].device_id)) {
			break;
		}
	}

	KASSERT(flash_ids[i].name != NULL);
	printf(": %s", flash_ids[i].name);
	if (humanize_number(nbuf, sizeof(nbuf), flash_ids[i].flash_size, "B",
	    1024) > 0)
		printf(" (%s)", nbuf);

	/*
	 * determine size of the largest block
	 */
	sc->sc_size = flash_ids[i].flash_size;
	sc->sc_sector_size = flash_ids[i].sector_size;

	if ((sc->sc_buf = malloc(sc->sc_sector_size, M_DEVBUF, M_NOWAIT))
	    == NULL) {
		printf(": can't alloc buffer space\n");
		return;
	}

	printf("\n");
}

int
flashopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct flash_softc	*sc;

	sc = device_lookup_private(&athflash_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;
	if (sc->sc_status & FLASH_ST_BUSY)
		return EBUSY;
	sc->sc_status |= FLASH_ST_BUSY;
	return 0;
}

int
flashclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct flash_softc	*sc;

	sc = device_lookup_private(&athflash_cd, minor(dev));
	sc->sc_status &= ~FLASH_ST_BUSY;
	return 0;
}

int
flashread(dev_t dev, struct uio *uio, int flag)
{
	struct flash_softc	*sc;
	bus_space_tag_t		iot;
	bus_space_handle_t	ioh;
	bus_size_t		off;
	int			total;
	int			count;
	int			error;

	sc = device_lookup_private(&athflash_cd, minor(dev));
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	off = uio->uio_offset;
	total = min(sc->sc_size - off, uio->uio_resid);

	while (total > 0) {
		count = min(sc->sc_sector_size, uio->uio_resid);
		bus_space_read_region_1(iot, ioh, off, sc->sc_buf, count);
		if ((error = uiomove(sc->sc_buf, count, uio)) != 0)
			return error;
		off += count;
		total -= count;
	}
	return 0;
}


int
flashwrite(dev_t dev, struct uio *uio, int flag)
{
	struct flash_softc	*sc;
	bus_space_tag_t		iot;
	bus_space_handle_t	ioh;
	bus_size_t		off;
	int			stat;
	int			error;

	sc = device_lookup_private(&athflash_cd, minor(dev));

	if (sc->sc_size < uio->uio_offset + uio->uio_resid)
		return ENOSPC;
	if (uio->uio_offset % sc->sc_sector_size)
		return EINVAL;
	if (uio->uio_resid % sc->sc_sector_size)
		return EINVAL;

	iot = sc->sc_iot;
	ioh = sc->sc_ioh;
	
	for (off = uio->uio_offset;
	     uio->uio_resid > 0;
	     off += sc->sc_sector_size) {
		error = uiomove(sc->sc_buf, sc->sc_sector_size, uio);
		if (error != 0)
			return error;
		if (is_block_same(sc, off, sc->sc_buf))
			continue;
		if ((stat = flash_sector_erase(sc, off)) != 0) {
			printf("sector erase failed status = 0x%x\n", stat);
			return EIO;
		}
		if ((stat = flash_sector_write(sc, off)) != 0) {
			printf("sector write failed status = 0x%x\n", stat);
			return EIO;
		}
	}
	return 0;
}

static int
is_block_same(struct flash_softc *sc, bus_size_t offset, const void *bufp)
{
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh = sc->sc_ioh;
	const u_int8_t		*p = bufp;
	int			count = sc->sc_sector_size;

	while (count-- > 0) {
		if (bus_space_read_1(iot, ioh, offset++) != *p++)
			return 0;
	}
	return 1;
}

static int
toggle_bit_wait(struct flash_softc *sc, bus_size_t offset,
    int typtmo, int maxtmo, int spin)
{
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh = sc->sc_ioh;
	uint8_t			d1, d2;

	while (maxtmo > 0) {

		if (spin) {
			DELAY(typtmo);
		} else {
			tsleep(sc, PRIBIO, "blockerase",
			    (typtmo / hz) + 1);
		}

		d1 = bus_space_read_1(iot, ioh, offset);
		d2 = bus_space_read_2(iot, ioh, offset);

		/* watch for the toggle bit to stop toggling */
		if ((d1 & 0x40) == (d2 & 0x40)) {
			return 0;
		}

		maxtmo -= typtmo;
	}
	return (ETIMEDOUT);
}

static int
flash_sector_erase(struct flash_softc *sc, bus_size_t offset)
{
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh = sc->sc_ioh;

	DPRINTF(("flash_sector_erase offset = %08lx\n", offset));

	bus_space_write_2(iot, ioh, (0x5555 << 1), 0xAAAA);
	bus_space_write_2(iot, ioh, (0x2AAA << 1), 0x5555);
	bus_space_write_2(iot, ioh, (0x5555 << 1), 0x8080);
	bus_space_write_2(iot, ioh, (0x5555 << 1), 0xAAAA);
	bus_space_write_2(iot, ioh, (0x2AAA << 1), 0x5555);

	bus_space_write_2(iot, ioh, offset, 0x3030);

	/*
	 * NB: with CFI, we could get more meaningful timeout data for
	 * now we just assign reasonable values - 10 msec typical, and
	 * up to 60 secs to erase the whole sector.
	 */

	return toggle_bit_wait(sc, offset, 10000, 60000000, 0);
}

static int
flash_sector_write(struct flash_softc *sc, bus_size_t offset)
{
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh = sc->sc_ioh;
	bus_size_t		fence;
	const u_int16_t		*p;

	p = (u_int16_t *) sc->sc_buf;
	fence = offset + sc->sc_sector_size;
	do {
		bus_space_write_2(iot, ioh, (0x5555 << 1), 0xAAAA);
		bus_space_write_2(iot, ioh, (0x2AAA << 1), 0x5555);
		bus_space_write_2(iot, ioh, (0xAAAA << 1), 0xA0A0);

		bus_space_write_2(iot, ioh, offset, *p);

		/* wait up to 1 msec, in 10 usec increments, no sleeping */
		if (toggle_bit_wait(sc, offset, 10, 1000, 1) != 0)
			return ETIMEDOUT;
		p++;
		offset += 2;
	} while (offset < fence);

	return 0;
}
