/*	$NetBSD: promlib.c,v 1.6 2005/12/24 23:24:06 perry Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 * Specially crafted scaled-down version of promlib for the first-stage
 * boot program.
 *
 * bootxx needs the follwoing PROM functions:
 *	prom_version()
 *	prom_getbootpath()
 *	prom_putchar()
 *	prom_halt()
 *	prom_open()
 *	prom_close()
 *	prom_seek()
 *	prom_read()
 */

#include <sys/errno.h>
#include <sys/param.h>

#include <machine/stdarg.h>
#include <machine/oldmon.h>
#include <machine/bsd_openprom.h>
#include <machine/promlib.h>

#include <lib/libsa/stand.h>

#define obpvec ((struct promvec *)romp)

static void	obp_v2_putchar __P((int));
static int	obp_v2_seek __P((int, u_quad_t));
static const char	*obp_v0_getbootpath __P((void));
static const char	*obp_v2_getbootpath __P((void));

/*
 * PROM entry points.
 */
struct promops promops;

/*
 * Determine whether a node has the given property.
 */
void
prom_halt()
{

	_prom_halt();
}


void
obp_v2_putchar(c)
	int c;
{
	char c0;

	c0 = (c & 0x7f);
	(*promops.po_write)(promops.po_stdout, &c0, 1);
}

int
obp_v2_seek(handle, offset)
	int handle;
	u_quad_t offset;
{
	u_int32_t hi, lo;

	lo = offset & ((u_int32_t)-1);
	hi = (offset >> 32) & ((u_int32_t)-1);
	(*obpvec->pv_v2devops.v2_seek)(handle, hi, lo);
	return (0);
}

/*
 * On SS1s (and also IPCs, SLCs), `promvec->pv_v0bootargs->ba_argv[1]'
 * contains the flags that were given after the boot command.  On SS2s
 * (and ELCs, IPXs, etc. and any sun4m class machine), `pv_v0bootargs'
 * is NULL but `*promvec->pv_v2bootargs.v2_bootargs' points to
 * "netbsd -s" or whatever.
 */
const char *
obp_v0_getbootpath()
{
	struct v0bootargs *ba = promops.po_bootcookie;
	return (ba->ba_argv[0]);
}

const char *
obp_v2_getbootpath()
{
	struct v2bootargs *ba = promops.po_bootcookie;
	return (*ba->v2_bootpath);
}


static void prom_init_oldmon __P((void));
static void prom_init_obp __P((void));

static inline void
prom_init_oldmon()
{
	struct om_vector *oldpvec = (struct om_vector *)PROM_BASE;

	promops.po_version = PROM_OLDMON;
	promops.po_revision = oldpvec->monId[0];	/*XXX*/

	promops.po_stdout = *oldpvec->outSink;
	promops.po_putchar = oldpvec->putChar;
	promops.po_halt = oldpvec->exitToMon;
	promops.po_bootcookie = *oldpvec->bootParam; /* deref 1 lvl */
	promops.po_bootpath = obp_v0_getbootpath;
}

static inline void
prom_init_obp()
{
	/*
	 * OBP v0, v2 & v3
	 */
	switch (obpvec->pv_romvec_vers) {
	case 0:
		promops.po_version = PROM_OBP_V0;
		break;
	case 2:
		promops.po_version = PROM_OBP_V2;
		break;
	case 3:
		promops.po_version = PROM_OBP_V3;
		break;
	default:
		obpvec->pv_halt();	/* What else? */
	}

	promops.po_revision = obpvec->pv_printrev;
	promops.po_halt = obpvec->pv_halt;

	/*
	 * Next, deal with prom vector differences between versions.
	 */
	switch (promops.po_version) {
	case PROM_OBP_V0:
		promops.po_stdout = *obpvec->pv_stdout;
		promops.po_putchar = obpvec->pv_putchar;
		promops.po_open = obpvec->pv_v0devops.v0_open;
		promops.po_close = (void *)obpvec->pv_v0devops.v0_close;
		promops.po_bootcookie = *obpvec->pv_v0bootargs; /* deref 1 lvl */
		promops.po_bootpath = obp_v0_getbootpath;
		break;
	case PROM_OBP_V3:
	case PROM_OBP_V2:
		/* Deref stdio handles one level */
		promops.po_stdout = *obpvec->pv_v2bootargs.v2_fd1;
		promops.po_putchar = obp_v2_putchar;
		promops.po_open = obpvec->pv_v2devops.v2_open;
		promops.po_close = (void *)obpvec->pv_v2devops.v2_close;
		promops.po_read = obpvec->pv_v2devops.v2_read;
		promops.po_write = obpvec->pv_v2devops.v2_write;
		promops.po_seek = obp_v2_seek;
		promops.po_bootcookie = &obpvec->pv_v2bootargs;
		promops.po_bootpath = obp_v2_getbootpath;
		break;
	}
}

/*
 * Initialize our PROM operations vector.
 */
void
prom_init()
{

	if (CPU_ISSUN4) {
		prom_init_oldmon();
		romp = (caddr_t)PROM_LOADADDR;	/* Used in main() */
	} else if (obpvec->pv_magic == OBP_MAGIC) {
		prom_init_obp();
	} else {
		/* No Openfirmware support */
	}
}
