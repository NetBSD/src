/*	$NetBSD: conf.c,v 1.8 1995/08/05 16:47:36 thorpej Exp $	*/

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

#include "stand.h"

#ifdef NETBOOT

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>

#include "net.h"
#include "netif.h"
#ifdef NFS
#include "nfs.h"
#endif
#ifdef UFS
#include "ufs.h"
#endif
#ifdef TFTP
#include "tftp.h"
#endif

#else

#define UFS
#include "ufs.h"

#endif

#ifdef UFS
/*
 * Device configuration
 */
#ifdef TAPEBOOT
#define	ctstrategy	\
	(int (*) __P((void *, int, daddr_t, u_int, char *, u_int *)))nullsys
#define	ctopen		(int (*) __P((struct open_file *, ...)))nodev
#define	ctclose		(int (*) __P((struct open_file *)))nullsys
#else
int	ctstrategy __P((void *, int, daddr_t, u_int, char *, u_int *));
int	ctopen __P((struct open_file *, ...));
int	ctclose __P((struct open_file *));
#endif
#define	ctioctl		noioctl

int	rdstrategy __P((void *, int, daddr_t, u_int, char *, u_int *));
int	rdopen __P((struct open_file *, ...));
#define rdclose		(int (*) __P((struct open_file *)))nullsys
#define rdioctl		noioctl

int	sdstrategy __P((void *, int, daddr_t, u_int, char *, u_int *));
int	sdopen __P((struct open_file *, ...));
#define sdclose		(int (*) __P((struct open_file *)))nullsys
#define	sdioctl		noioctl

#define xxstrategy	\
	(int (*) __P((void *, int, daddr_t, u_int, char *, u_int *)))nullsys
#define xxopen		(int (*) __P((struct open_file *, ...)))nodev
#define xxclose		(int (*) __P((struct open_file *)))nullsys

struct devsw devsw[] = {
	{ "ct",	ctstrategy,	ctopen,	ctclose,	ctioctl }, /*0*/
	{ "??",	xxstrategy,	xxopen,	xxclose,	noioctl }, /*1*/
	{ "rd",	rdstrategy,	rdopen,	rdclose,	rdioctl }, /*2*/
	{ "??",	xxstrategy,	xxopen,	xxclose,	noioctl }, /*3*/
	{ "sd",	sdstrategy,	sdopen,	sdclose,	sdioctl }, /*4*/
};
int	ndevs = (sizeof(devsw)/sizeof(devsw[0]));

#else

struct devsw devsw[];
int	ndevs = 0;

#endif


/*
 * Filesystem configuration
 */
struct fs_ops file_system[] = {
#ifdef UFS
	{ ufs_open, ufs_close, ufs_read, ufs_write, ufs_seek, ufs_stat },
#else
	{ null_open, null_close, null_read, null_write, null_seek, null_stat },
#endif
#ifdef NFS
	{ nfs_open, nfs_close, nfs_read, nfs_write, nfs_seek, nfs_stat },
#else
	{ null_open, null_close, null_read, null_write, null_seek, null_stat },
#endif
#ifdef TFTP
	{ tftp_open, tftp_close, tftp_read, null_write, null_seek, null_stat },
#else
	{ null_open, null_close, null_read, null_write, null_seek, null_stat },
#endif
};

int nfsys = (sizeof(file_system) / sizeof(struct fs_ops));

#ifdef NETBOOT
extern struct netif_driver le_driver;

struct netif_driver *netif_drivers[] = {
	&le_driver,
};
int n_netif_drivers = sizeof(netif_drivers)/sizeof(netif_drivers[0]);
#endif

	
/*
 * Inititalize controllers
 * 
 * XXX this should be a table
 */
void ctlrinit()
{
#ifdef NETBOOT
	leinit();
#else
	hpibinit();
	scsiinit();
#endif
}
