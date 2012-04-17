/*	$NetBSD: sdhcvar.h,v 1.5.2.1 2012/04/17 00:08:03 yamt Exp $	*/
/*	$OpenBSD: sdhcvar.h,v 1.3 2007/09/06 08:01:01 jsg Exp $	*/

/*
 * Copyright (c) 2006 Uwe Stuehler <uwe@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SDHCVAR_H_
#define _SDHCVAR_H_

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/pmf.h>

struct sdhc_host;

struct sdhc_softc {
	device_t		sc_dev;

	struct sdhc_host	**sc_host;
	int			sc_nhosts;

	bus_dma_tag_t		sc_dmat;

	uint32_t		sc_flags;
#define	SDHC_FLAG_USE_DMA	0x0001
#define	SDHC_FLAG_FORCE_DMA	0x0002
#define	SDHC_FLAG_NO_PWR0	0x0004	/* Freescale ESDHC */
#define	SDHC_FLAG_HAVE_DVS	0x0008	/* Freescale ESDHC */
#define	SDHC_FLAG_32BIT_ACCESS	0x0010	/* Freescale ESDHC */
#define	SDHC_FLAG_ENHANCED	0x0020	/* Freescale ESDHC */
#define	SDHC_FLAG_8BIT_MODE	0x0040	/* MMC 8bit mode is supported */
#define	SDHC_FLAG_HAVE_CGM	0x0080	/* Netlogic XLP */
	uint32_t		sc_clkbase;
};

/* Host controller functions called by the attachment driver. */
int	sdhc_host_found(struct sdhc_softc *, bus_space_tag_t,
	    bus_space_handle_t, bus_size_t);
int	sdhc_intr(void *);
int	sdhc_detach(device_t, int);
bool	sdhc_suspend(device_t, const pmf_qual_t *);
bool	sdhc_resume(device_t, const pmf_qual_t *);
bool	sdhc_shutdown(device_t, int);

#endif	/* _SDHCVAR_H_ */
