/*	$NetBSD: conf.c,v 1.12 2007/12/23 03:11:32 tsutsui Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)conf.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/nfs.h>
#include <lib/libsa/ufs.h>

#include <hp300/stand/common/conf.h>
#include <hp300/stand/common/rawfs.h>
#include <hp300/stand/common/samachdep.h>

int	debug = 0;	/* XXX */

#define xxstrategy	\
	(int (*)(void *, int, daddr_t, size_t, void *, size_t *))nullsys
#define xxopen		(int (*)(struct open_file *, ...))nodev
#define xxclose		(int (*)(struct open_file *))nullsys

/*
 * Device configuration
 */
#ifndef SUPPORT_ETHERNET
#define	netstrategy	xxstrategy
#define	netopen		xxopen
#define	netclose	xxclose
#endif
#define	netioctl	noioctl

#ifndef SUPPORT_TAPE
#define	ctstrategy	xxstrategy
#define	ctopen		xxopen
#define	ctclose		xxclose
#endif
#define	ctioctl		noioctl

#ifndef SUPPORT_DISK
#define	rdstrategy	xxstrategy
#define	rdopen		xxopen
#define	rdclose		xxclose
#define	sdstrategy	xxstrategy
#define	sdopen		xxopen
#define	sdclose		xxclose
#endif
#define rdioctl		noioctl
#define	sdioctl		noioctl

/*
 * Note: "le" isn't a major offset.
 */
struct devsw devsw[] = {
	{ "ct",	ctstrategy,	ctopen,	ctclose,	ctioctl }, /*0*/
	{ "??",	xxstrategy,	xxopen,	xxclose,	noioctl }, /*1*/
	{ "rd",	rdstrategy,	rdopen,	rdclose,	rdioctl }, /*2*/
	{ "??",	xxstrategy,	xxopen,	xxclose,	noioctl }, /*3*/
	{ "sd",	sdstrategy,	sdopen,	sdclose,	sdioctl }, /*4*/
	{ "??",	xxstrategy,	xxopen,	xxclose,	noioctl }, /*5*/
	{ "le",	netstrategy,	netopen, netclose,	netioctl },/*6*/
};
int	ndevs = __arraycount(devsw);

#ifdef SUPPORT_ETHERNET
struct netif_driver *netif_drivers[] = {
	&le_driver,
};
int	n_netif_drivers = __arraycount(netif_drivers);
#endif

/*
 * Physical unit/lun detection.
 */
static int punitzero(int, int, int *);

static int
punitzero(int ctlr, int slave, int *punit)
{

	*punit = 0;
	return 0;
}

#define	xxpunit		punitzero
#ifndef SUPPORT_TAPE
#define	ctpunit		xxpunit
#endif
#define	rdpunit		punitzero
#define	sdpunit		punitzero
#define	lepunit		punitzero

struct punitsw punitsw[] = {
	{ ctpunit },
	{ xxpunit },
	{ rdpunit },
	{ xxpunit },
	{ sdpunit },
	{ xxpunit },
	{ lepunit },
};
int	npunit = __arraycount(punitsw);

/*
 * Filesystem configuration
 */
struct fs_ops file_system_rawfs[] = { FS_OPS(rawfs) };
struct fs_ops file_system_ufs[] = { FS_OPS(ufs) };
struct fs_ops file_system_nfs[] = { FS_OPS(nfs) };

struct fs_ops file_system[1];
int	nfsys = 1;		/* we always know which one we want */

#if 0
/*
 * Inititalize controllers
 *
 * XXX this should be a table
 */
void ctlrinit(void)
{
#ifdef SUPPORT_ETHERNET
	leinit();
#endif
#if defined(SUPPORT_DISK) || defined(SUPPORT_TAPE)
	hpibinit();
	scsiinit();
#endif
}
#endif
