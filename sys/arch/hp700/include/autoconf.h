/*	$NetBSD: autoconf.h,v 1.13 2009/11/03 05:07:25 snj Exp $	*/

/*	$OpenBSD: autoconf.h,v 1.10 2001/05/05 22:33:42 art Exp $	*/

/*
 * Copyright (c) 1998-2004 Michael Shalayeff
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/device.h>

#include <machine/bus.h>
#include <machine/pdc.h>

struct confargs {
	struct iodc_data ca_type PDC_ALIGNMENT;	/* iodc-specific type descrition */
	struct device_path ca_dp;	/* device_path as found by pdc_scan */
	struct pdc_iodc_read *ca_pdc_iodc_read;
	struct {
		hppa_hpa_t addr;
		u_int   size;
	}		ca_addrs[16];	/* 16 is ought to be enough */
	const char	*ca_name;	/* device name/description */
	bus_space_tag_t	ca_iot;		/* io tag */
	int		ca_mod;		/* module number on the bus */
	hppa_hpa_t	ca_hpa;		/* module HPA */
	u_int		ca_hpasz;	/* module HPA size (if avail) */
	bus_dma_tag_t	ca_dmatag;	/* DMA tag */
	int		ca_irq;		/* module IRQ */
	int		ca_naddrs;	/* number of valid addr ents */
	hppa_hpa_t	ca_hpabase;	/* HPA base to use or 0 for PDC */
	int		ca_nmodules;	/* check for modules 0 to nmodules - 1 */
};

#define	HP700CF_IRQ_UNDEF	(-1)
#define	hp700cf_irq	cf_loc[GEDOENSCF_IRQ]

/* this is used for hppa_knownmodules table
 * describing known to this port modules,
 * system boards, cpus, fpus and busses
 */
struct hppa_mod_info {
	int	mi_type;
	int	mi_sv;
	const char *mi_name;
};

extern void (*cold_hook)(int);
#define	HPPA_COLD_COLD	0
#define	HPPA_COLD_HOT	1
#define	HPPA_COLD_OFF	2

const char *hppa_mod_info(int, int);

void	pdc_scanbus(device_t, struct confargs *,
    void (*)(device_t, struct confargs *));

int	mbprint(void *, const char *);
int	mbsubmatch(device_t, struct cfdata *,
			const int *, void *);
int	clock_intr(void *);

void	dumpconf(void);
