/*	$NetBSD: eeprom.c,v 1.33.8.1 2020/12/14 14:38:03 thorpej Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: eeprom.c,v 1.33.8.1 2020/12/14 14:38:03 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/kernel.h>

#include <machine/autoconf.h>
#include <machine/idprom.h>
#include <machine/eeprom.h>

#ifndef EEPROM_SIZE
#define EEPROM_SIZE 0x800
#endif

struct eeprom *eeprom_copy; 	/* soft copy. */

static int ee_update(int off, int cnt);

static uint8_t *eeprom_va; /* mapping to actual device */
static int ee_size;     /* size of usable part. */

static int ee_busy, ee_want; /* serialization */

static int  eeprom_match(device_t, cfdata_t, void *);
static void eeprom_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(eeprom, 0,
    eeprom_match, eeprom_attach, NULL, NULL);

static int 
eeprom_match(device_t parent, cfdata_t cf, void *args)
{
	struct confargs *ca = args;

	/* This driver only supports one instance. */
	if (eeprom_va != NULL)
		return 0;

	if (bus_peek(ca->ca_bustype, ca->ca_paddr, 1) == -1)
		return 0;

	return 1;
}

static void 
eeprom_attach(device_t parent, device_t self, void *args)
{
	struct confargs *ca = args;
	uint8_t *src, *dst, *lim;

	aprint_normal("\n");
#ifdef	DIAGNOSTIC
	if (sizeof(struct eeprom) != EEPROM_SIZE)
		panic("eeprom struct wrong");
#endif

	ee_size = EEPROM_SIZE;
	eeprom_va = bus_mapin(ca->ca_bustype, ca->ca_paddr, ee_size);
	if (eeprom_va == NULL)
		panic("%s: can't map va", __func__);

	/* Keep a "soft" copy of the EEPROM to make access simpler. */
	eeprom_copy = kmem_alloc(ee_size, KM_SLEEP);

	/*
	 * On the 3/80, do not touch the last 40 bytes!
	 * Writes there are ignored, reads show zero.
	 * Reduce ee_size and clear the last part of the
	 * soft copy.  Note: ee_update obeys ee_size.
	 */
	if (cpu_machine_id == ID_SUN3X_80)
		ee_size -= 40;

	/* Do only byte access in the EEPROM. */
	src = eeprom_va;
	dst = (uint8_t *)eeprom_copy;
	lim = dst + ee_size;

	do {
		*dst++ = *src++;
	} while (dst < lim);

	if (ee_size < EEPROM_SIZE) {
		/* Clear out the last part. */
		memset(dst, 0, (EEPROM_SIZE - ee_size));
	}
}


/* Take the lock. */
static int
ee_take(void)
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
ee_give(void)
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
	uint8_t *va;

	if (eeprom_copy == NULL)
		return ENXIO;

	off = uio->uio_offset;
	if ((off < 0) || (off > EEPROM_SIZE))
		return EFAULT;

	cnt = uimin(uio->uio_resid, (EEPROM_SIZE - off));
	if (cnt == 0)
		return 0;	/* EOF */

	va = ((uint8_t *)eeprom_copy) + off;
	error = uiomove(va, (int)cnt, uio);

	/* If we wrote the rambuf, update the H/W. */
	if (error == 0 && (uio->uio_rw != UIO_READ)) {
		error = ee_take();
		if (error == 0)
			error = ee_update(off, cnt);
		ee_give();
	}

	return error;
}

/*
 * Update the EEPROM from the soft copy.
 * Other than the attach function, this is
 * the ONLY place we touch the EEPROM H/W.
 */
static int
ee_update(int off, int cnt)
{
	volatile uint8_t *ep;
	uint8_t *bp;
	int errcnt;

	if (eeprom_va == NULL)
		return (ENXIO);

	/*
	 * This check is NOT redundant with the one in
	 * eeprom_uio above if we are on a 3/80 where
	 * ee_size < EEPROM_SIZE
	 */
	if (cnt > (ee_size - off))
		cnt = (ee_size - off);

	bp = ((uint8_t *)eeprom_copy) + off;
	ep = eeprom_va + off;
	errcnt = 0;

	while (cnt > 0) {
		/*
		 * DO NOT WRITE IT UNLESS WE HAVE TO because the
		 * EEPROM has a limited number of write cycles.
		 * After some number of writes it just fails!
		 */
		if (*ep != *bp) {
			*ep = *bp;
			/*
			 * We have written the EEPROM, so now we must
			 * sleep for at least 10 milliseconds while
			 * holding the lock to prevent all access to
			 * the EEPROM while it recovers.
			 */
			(void)tsleep(eeprom_va, PZERO - 1, "eeprom", hz / 50);
		}
		/* Make sure the write worked. */
		if (*ep != *bp)
			errcnt++;
		ep++;
		bp++;
		cnt--;
	}
	return errcnt ? EIO : 0;
}
