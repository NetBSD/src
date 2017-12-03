/*	$NetBSD: ibus_pmax.c,v 1.23.12.1 2017/12/03 11:36:35 jdolecek Exp $	*/

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: ibus_pmax.c,v 1.23.12.1 2017/12/03 11:36:35 jdolecek Exp $");

#include "opt_dec_3100.h"
#include "opt_dec_5100.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <pmax/ibus/ibusvar.h>

#include <pmax/autoconf.h>
#include <pmax/sysconf.h>

#include <pmax/pmax/kn01.h>
#include <pmax/pmax/kn230.h>
#include <pmax/pmax/pmaxtype.h>

static int	ibus_pmax_match(device_t, cfdata_t, void *);
static void	ibus_pmax_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ibus_pmax, 0,
    ibus_pmax_match, ibus_pmax_attach, NULL, NULL);

#define KV(x)	MIPS_PHYS_TO_KSEG1(x)

#ifdef DEC_3100
static struct ibus_attach_args ibus_pmax_devs[] = {
        { "pm",         SYS_DEV_BOGUS,	KV(KN01_PHYS_FBUF_START), 0	},
        { "dc",         SYS_DEV_SCC0,	KV(KN01_SYS_DZ),        0	},
        { "lance",      SYS_DEV_LANCE,	KV(KN01_SYS_LANCE),     0	},
        { "sii",        SYS_DEV_SCSI,	KV(KN01_SYS_SII),       0	},
        { "mc146818",   SYS_DEV_BOGUS,	KV(KN01_SYS_CLOCK),     0	},
};
#endif /* DEC_3100 */

#ifdef DEC_5100
static struct ibus_attach_args ibus_mipsmate_devs[] = {
	{ "dc",		SYS_DEV_SCC0,	KV(KN230_SYS_DZ0),	0 },
	{ "lance",	SYS_DEV_LANCE,	KV(KN230_SYS_LANCE),	0 },
	{ "sii",	SYS_DEV_SCSI,	KV(KN230_SYS_SII),	0 },
	{ "mc146818",	SYS_DEV_BOGUS,	KV(KN230_SYS_CLOCK),	0 },
#if 0	/* 5100 locks up when these are probed at the moment */
	{ "dc",		SYS_DEV_OPT0,	KV(KN230_SYS_DZ1),	0 },
	{ "dc",		SYS_DEV_OPT1,	KV(KN230_SYS_DZ2),	0 },
	/*
	 * Ultrix configures it at 0x86400400.  The first 0x400 bytes
	 * used for NVRAM state??
	 *
	 * The first 0x400 bytes are apparently used for diagnostic
	 * registers - ad
	 */
	{ "nvram",	SYS_DEV_BOGUS,	KV(0x86400000),		0 },
#endif
};
#endif /* DEC_5100 */

static int ibus_attached;

static int
ibus_pmax_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (ibus_attached)
		return (0);
	if (systype != DS_PMAX && systype != DS_MIPSMATE)
		return (0);
	if (strcmp(ma->ma_name, "baseboard") != 0)
		return (0);

	return (1);
}

static void
ibus_pmax_attach(device_t parent, device_t self, void *aux)
{
	struct ibus_dev_attach_args ida;

	ibus_attached = 1;

	ida.ida_busname = "ibus";
	ida.ida_memt = normal_memt;
	switch (systype) {
#ifdef DEC_3100
	case DS_PMAX:
		ida.ida_devs = ibus_pmax_devs;
		ida.ida_ndevs = __arraycount(ibus_pmax_devs);
		break;
#endif
#ifdef DEC_5100
	case DS_MIPSMATE:
		ida.ida_devs = ibus_mipsmate_devs;
		ida.ida_ndevs = __arraycount(ibus_mipsmate_devs);
		break;
#endif
	default:
		panic("ibus_pmax_attach: no ibus configured for systype = %d", systype);
	}

	ibusattach(parent, self, &ida);
}
