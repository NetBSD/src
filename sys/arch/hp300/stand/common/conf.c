/*	$NetBSD: conf.c,v 1.4 1999/12/14 20:55:27 thorpej Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include <hp300/stand/common/rawfs.h>
#include <hp300/stand/common/samachdep.h>

int	debug = 0;	/* XXX */

#define xxstrategy	\
	(int (*) __P((void *, int, daddr_t, size_t, void *, size_t *)))nullsys
#define xxopen		(int (*) __P((struct open_file *, ...)))nodev
#define xxclose		(int (*) __P((struct open_file *)))nullsys

/*
 * Device configuration
 */
#ifdef SUPPORT_ETHERNET
int	netstrategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int	netopen __P((struct open_file *, ...));
int	netclose __P((struct open_file *));
#define netioctl	noioctl
#else
#define	netstrategy	xxstrategy
#define	netopen		xxopen
#define	netclose	xxclose
#define	netioctl	noioctl
#endif

#ifdef SUPPORT_TAPE
int	ctstrategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int	ctopen __P((struct open_file *, ...));
int	ctclose __P((struct open_file *));
#define	ctioctl		noioctl
#else
#define	ctstrategy	xxstrategy
#define	ctopen		xxopen
#define	ctclose		xxclose
#define	ctioctl		noioctl
#endif

#ifdef SUPPORT_DISK
int	rdstrategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int	rdopen __P((struct open_file *, ...));
int	rdclose __P((struct open_file *));
#define rdioctl		noioctl
#else
#define	rdstrategy	xxstrategy
#define	rdopen		xxopen
#define	rdclose		xxclose
#define	rdioctl		noioctl
#endif

#ifdef SUPPORT_DISK
int	sdstrategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int	sdopen __P((struct open_file *, ...));
int	sdclose __P((struct open_file *));
#define	sdioctl		noioctl
#else
#define	sdstrategy	xxstrategy
#define	sdopen		xxopen
#define	sdclose		xxclose
#define	sdioctl		noioctl
#endif

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
int	ndevs = (sizeof(devsw) / sizeof(devsw[0]));

#ifdef SUPPORT_ETHERNET
extern struct netif_driver le_driver;

struct netif_driver *netif_drivers[] = {
	&le_driver,
};
int	n_netif_drivers = (sizeof(netif_drivers) / sizeof(netif_drivers[0]));
#endif

/*
 * Physical unit/lun detection.
 */
int	punitzero __P((int, int, int *));

int
punitzero(ctlr, slave, punit)
	int ctlr, slave, *punit;
{

	*punit = 0;
	return (0);
}

#define	xxpunit		punitzero
#ifdef SUPPORT_TAPE
extern int ctpunit __P((int, int, int *));
#else
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
int	npunit = (sizeof(punitsw) / sizeof(punitsw[0]));

/*
 * Filesystem configuration
 */
struct fs_ops file_system_rawfs[] = {
	{ rawfs_open, rawfs_close, rawfs_read, rawfs_write, rawfs_seek,
	    rawfs_stat },
};

struct fs_ops file_system_ufs[] = {
	{ ufs_open, ufs_close, ufs_read, ufs_write, ufs_seek, ufs_stat },
};

struct fs_ops file_system_nfs[] = {
	{ nfs_open, nfs_close, nfs_read, nfs_write, nfs_seek, nfs_stat },
};

struct fs_ops file_system[1];
int	nfsys = 1;		/* we always know which one we want */


/*
 * Inititalize controllers
 * 
 * XXX this should be a table
 */
void ctlrinit()
{
#ifdef SUPPORT_ETHERNET
	leinit();
#endif
#if defined(SUPPORT_DISK) || defined(SUPPORT_TAPE)
	hpibinit();
	scsiinit();
#endif
}
