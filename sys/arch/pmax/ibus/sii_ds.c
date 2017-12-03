/*	$NetBSD: sii_ds.c,v 1.9.12.1 2017/12/03 11:36:35 jdolecek Exp $	*/

/*
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * this driver contributed by Jonathan Stone
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sii_ds.c,v 1.9.12.1 2017/12/03 11:36:35 jdolecek Exp $");

#include "sii.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <mips/locore.h>
#include <pmax/locore.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <pmax/ibus/siireg.h>
#include <pmax/ibus/siivar.h>

#include <pmax/ibus/ibusvar.h>		/* interrupt etablish */

#include <pmax/pmax/kn01.h>		/* kn01 (ds3100) address constants */
#include <pmax/pmax/pmaxtype.h>


static void	kn230_copytobuf(u_short *src, 	/* NB: must be short aligned */
		    volatile u_short *dst, int length);
static void	kn230_copyfrombuf(volatile u_short *src, char *dst,
		    int length);

static void	kn01_copytobuf(u_short *src, 	/* NB: must be short aligned */
		    volatile u_short *dst, int length);
static void	kn01_copyfrombuf(volatile u_short *src, char *dst,
		    int length);

/*
 * Autoconfig definition of driver front-end
 */
static int	sii_ds_match(device_t, struct cfdata *, void *);
static void	sii_ds_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sii_ds, sizeof(struct siisoftc),
    sii_ds_match, sii_ds_attach, NULL, NULL);

/* define a safe address in the SCSI buffer for doing status & message DMA */
#define SII_BUF_ADDR	(MIPS_PHYS_TO_KSEG1(KN01_SYS_SII_B_START) \
		+ SII_MAX_DMA_XFER_LENGTH * 14)

/*
 * Match driver on Decstation (2100, 3100, 5100) based on name and probe.
 */
static int
sii_ds_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ibus_attach_args *ia = aux;
	void *siiaddr;

	if (strcmp(ia->ia_name, "sii") != 0)
		return (0);
	siiaddr = (void *)ia->ia_addr;
	if (badaddr(siiaddr, 4))
		return (0);
	return (1);
}

static void
sii_ds_attach(device_t parent, device_t self, void *aux)
{
	struct ibus_attach_args *ia = aux;
	struct siisoftc *sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_regs = (SIIRegs *)MIPS_PHYS_TO_KSEG1(ia->ia_addr);

	/* set up scsi buffer.  XXX Why statically allocated? */
	sc->sc_buf = (void*)(MIPS_PHYS_TO_KSEG1(KN01_SYS_SII_B_START));

	if (systype == DS_PMAX) {
#if 0
		sc->sii_copytobuf = CopyToBuffer;
		sc->sii_copyfrombuf = CopyFromBuffer;
#else
		sc->sii_copytobuf = kn01_copytobuf;
		sc->sii_copyfrombuf = kn01_copyfrombuf;
#endif
	} else {
		sc->sii_copytobuf = kn230_copytobuf;
		sc->sii_copyfrombuf = kn230_copyfrombuf;
	}

	/* Do the common parts of attachment. */
	sc->sc_adapter.adapt_request = sii_scsi_request;
	sc->sc_adapter.adapt_minphys = minphys;
	siiattach(sc);

	/* tie pseudo-slot to device */
	ibus_intr_establish(parent, (void*)ia->ia_cookie, IPL_BIO, siiintr, sc);
}


/*
 * Padded DMA copy functions
 *
 */

/*
 * XXX assumes src is always 32-bit aligned.
 * currently safe on sii driver, but API and casts should be changed.
 */
static void
kn230_copytobuf(u_short *src, volatile u_short *dst, int len)
{
	u_int *wsrc = (u_int *)src;
	volatile u_int *wdst = (volatile u_int *)dst;
	int i, n;

#if defined(DIAGNOSTIC) || defined(DEBUG)
	if ((u_int)(src) & 0x3) {
		printf("kn230: copytobuf, src %p misaligned\n",  src);
	}
	if ((u_int)(dst) & 0x3) {
		printf("kn230: copytobuf, dst %p misaligned\n",  dst);
	}
#endif

	/* DMA buffer is allocated in 32-bit words, so just copy words. */
	n = len / 4;
	if (len & 0x3)
		n++;
	for (i = 0; i < n; i++) {
		*wdst = *wsrc;
		wsrc++;
		wdst+= 2;
	}

	wbflush();		/* XXX not necessary? */
}


/*
 * XXX assumes dst is always 32-bit aligned.
 * currently safe on sii driver, but API and casts should be changed.
 */
static void
kn230_copyfrombuf(volatile u_short *src, char *dst, int len)
	/* dst:		 XXX assume 32-bit aligned? */
{
	volatile u_int *wsrc = (volatile u_int *)src;
	u_int *wdst = (u_int *)dst;
	int i, n;

#if defined(DIAGNOSTIC) || defined(DEBUG)
	if ((u_int)(src) & 0x3) {
		printf("kn230: copyfrombuf, src %p misaligned\n",  src);
	}
	if ((u_int)(dst) & 0x3) {
		printf("kn230: copyfrombuf, dst %p misaligned\n",  dst);
	}
#endif

	n = len / 4;

	for (i = 0; i < n; i++) {
		*wdst = *wsrc;
		wsrc += 2;
		wdst++;
	}

	if (len & 0x3) {
		u_int lastword = *wsrc;

		if (len & 0x2)
		    *((u_short*)(wdst)) = (u_short) (lastword);

		if (len & 0x1)
		    ((u_char*)(wdst))[2] = (u_char) (lastword >> 16);
	}

	wbflush();		/* XXX not necessary? */
}


static void
kn01_copytobuf(u_short *src, volatile u_short *dst, int len)
{
#if defined(DIAGNOSTIC) || defined(DEBUG)
	if ((u_int)(src) & 0x3) {
		printf("kn01: copytobuf, src %p misaligned\n",  src);
	}
	if ((u_int)(dst) & 0x3) {
		printf("kn01: copytobuf, dst %p misaligned\n",  dst);
	}
#endif

	CopyToBuffer(src, dst, len);
}

static void
kn01_copyfrombuf(volatile u_short *src, char *dst, int len)
	/* dst:		 XXX assume 32-bit aligned? */
{
#if defined(DIAGNOSTIC) || defined(DEBUG)
	if ((u_int)(src) & 0x3) {
		printf("kn01: copyfrombuf, src %p misaligned\n",  src);
	}
	if ((u_int)(dst) & 0x3) {
		printf("kn01: copyfrombuf, dst %p misaligned\n",  dst);
	}

#endif
	CopyFromBuffer(src, dst, len);

}
