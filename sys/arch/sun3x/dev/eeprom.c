/*	$NetBSD: eeprom.c,v 1.3 1997/04/25 18:57:49 gwr Exp $	*/

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

/*
 * Access functions for the EEPROM (Electrically Eraseable PROM)
 * The main reason for the existence of this module is to
 * handle the painful task of updating the EEPROM contents.
 * After a write, it must not be touched for 10 milliseconds.
 * (See the Sun-3 Architecture Manual sec. 5.9)
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <machine/autoconf.h>
#include <machine/obio.h>
#include <machine/eeprom.h>
#include <machine/machdep.h>

#define HZ 100	/* XXX */

#ifndef OBIO_EEPROM_SIZE
#define OBIO_EEPROM_SIZE sizeof(struct eeprom)
#endif

static int ee_update(int off, int cnt);

static char *eeprom_va;
static char *ee_rambuf;
static int ee_busy, ee_want;

static int  eeprom_match __P((struct device *, struct cfdata *, void *));
static void eeprom_attach __P((struct device *, struct device *, void *));

struct cfattach eeprom_ca = {
	sizeof(struct device), eeprom_match, eeprom_attach
};

struct cfdriver eeprom_cd = {
	NULL, "eeprom", DV_DULL
};

static int
eeprom_match(parent, cf, args)
    struct device *parent;
	struct cfdata *cf;
    void *args;
{
	struct confargs *ca = args;

	/* This driver only supports one unit. */
	if (cf->cf_unit != 0)
		return (0);

	/* Validate the given address. */
	if (ca->ca_paddr != OBIO_EEPROM)
		return (0);

	return (1);
}

static void
eeprom_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
	char *src, *dst, *lim;

	printf("\n");

	eeprom_va = obio_mapin(OBIO_EEPROM, OBIO_EEPROM_SIZE);
	if (!eeprom_va)
		panic("eeprom_attach");

	/* Keep a "soft" copy of the EEPROM to make access simpler. */
	ee_rambuf = malloc(sizeof(struct eeprom), M_DEVBUF, M_NOWAIT);
	if (ee_rambuf == 0)
		panic("eeprom_attach: malloc ee_rambuf");

	/* Do only byte access in the EEPROM. */
	src = eeprom_va;
	dst = ee_rambuf;
	lim = ee_rambuf + sizeof(struct eeprom);
	do *dst++ = *src++;
	while (dst < lim);
}


/* Take the lock. */
static int
ee_take __P((void))
{
	int error = 0;
	while (ee_busy) {
		ee_want = 1;
		error = tsleep(&ee_busy, PZERO | PCATCH, "eeprom", 0);
		ee_want = 0;
		if (error)	/* interrupted */
			return error;
	}
	ee_busy = 1;
	return error;
}

/* Give the lock. */
static void
ee_give __P((void))
{
	ee_busy = 0;
	if (ee_want) {
		ee_want = 0;
		wakeup(&ee_busy);
	}
}

/*
 * This is called by mem.c to handle /dev/eeprom
 */
int
eeprom_uio(struct uio *uio)
{
	int cnt, error;
	int off;	/* NOT off_t */
	caddr_t va;

	if (ee_rambuf == NULL)
		return (ENXIO);

	off = uio->uio_offset;
	if ((off < 0) || (off > OBIO_EEPROM_SIZE))
		return (EFAULT);

	cnt = min(uio->uio_resid, (OBIO_EEPROM_SIZE - off));
	if (cnt == 0)
		return (0);	/* EOF */

	va = ee_rambuf + off;
	error = uiomove(va, (int)cnt, uio);

	/* If we wrote the rambuf, update the H/W. */
	if (!error && (uio->uio_rw != UIO_READ)) {
		error = ee_take();
		if (!error)
			error = ee_update(off, cnt);
		ee_give();
	}

	return (error);
}

/*
 * Update the EEPROM from the soft copy.
 * Other than the attach function, this is
 * the ONLY place we touch the EEPROM H/W.
 */
static int
ee_update(int off, int cnt)
{
	volatile char *ep;
	char *bp;
	int errcnt;

	if (eeprom_va == NULL)
		return (ENXIO);

	bp = ee_rambuf + off;
	ep = eeprom_va + off;
	errcnt = 0;

	while (cnt > 0) {
		/*
		 * DO NOT WRITE IT UNLESS WE HAVE TO because the
		 * EEPROM has a limited number of write cycles.
		 * After some number of writes it just fails!
		 */
		if (*ep != *bp) {
			*ep  = *bp;
			/*
			 * We have written the EEPROM, so now we must
			 * sleep for at least 10 milliseconds while
			 * holding the lock to prevent all access to
			 * the EEPROM while it recovers.
			 */
			(void)tsleep(eeprom_va, PZERO-1, "eeprom", HZ/50);
		}
		/* Make sure the write worked. */
		if (*ep != *bp)
			errcnt++;
		ep++;
		bp++;
		cnt--;
	}
	return (errcnt ? EIO : 0);
}
