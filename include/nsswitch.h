/*	$NetBSD: nsswitch.h,v 1.1.4.2 1997/05/23 20:31:42 lukem Exp $	*/

/*-
 * Copyright (c) 1997 Luke Mewburn <lukem@netbsd.org> 
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by Luke Mewburn.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef _NSSWITCH_H
#define _NSSWITCH_H	1

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <sys/types.h>

#ifndef _PATH_NS_CONF
#define _PATH_NS_CONF	"/etc/nsswitch.conf"
#endif

#define	NS_CONTINUE	0
#define	NS_RETURN	1

#define	NS_SUCCESS	0x10	/* entry was found */
#define	NS_UNAVAIL	0x20	/* source not responding, or corrupt */
#define	NS_NOTFOUND	0x40	/* source responded 'no such entry' */
#define	NS_TRYAGAIN	0x80	/* source busy, may respond to retrys */
#define	NS_STATUSMASK	0xf0 	/* bitmask to get the status */

#define NS_FILES	0x00	/* local files */
#define	NS_DNS		0x01	/* DNS; class IN for hosts, HS for others */
#define	NS_NIS		0x02	/* yp/nis */
#define	NS_NISPLUS	0x03	/* nis+ */
#define	NS_COMPAT	0x04	/* passwd,group in yp compat mode */
#define	NS_MAXSOURCE	0x0f	/* last possible source */
#define	NS_SOURCEMASK	0x0f	/* bitmask to get the source */

/*
 * currently implemented databases
 */
#define NSDB_HOSTS		"hosts"
#define NSDB_GROUP		"group"
#define NSDB_GROUP_COMPAT	"group_compat"
#define NSDB_NETGROUP		"netgroup"
#define NSDB_PASSWD		"passwd"
#define NSDB_PASSWD_COMPAT	"passwd_compat"
#define NSDB_SHELLS		"shells"

/*
 * suggested databases to implement
 */
#define NSDB_ALIASES		"aliases"
#define NSDB_AUTH		"auth"
#define NSDB_AUTOMOUNT		"automount"
#define NSDB_BOOTPARAMS		"bootparams"
#define NSDB_ETHERS		"ethers"
#define NSDB_EXPORTS		"exports"
#define NSDB_NETMASKS		"netmasks"
#define NSDB_NETWORKS		"networks"
#define NSDB_PHONES		"phones"
#define NSDB_PRINTCAP		"printcap"
#define NSDB_PROTOCOLS		"protocols"
#define NSDB_REMOTE		"remote"
#define NSDB_RPC		"rpc"
#define NSDB_SENDMAILVARS	"sendmailvars"
#define NSDB_SERVICES		"services"
#define NSDB_TERMCAP		"termcap"
#define NSDB_TTYS		"ttys"

#define NS_MAXDBLEN	32	/* max len of a database name */

typedef struct {
	int	(*cb)(void *retval, void *cb_data, va_list ap);
	void	*cb_data;
} ns_dtab [NS_MAXSOURCE];

typedef struct {
	char	name[NS_MAXDBLEN];	/* name of database */
	int	size;			/* number of entries of map */
	u_char	map[NS_MAXSOURCE];	/* map, described below */
} ns_DBT;
/*
 * ns_DBT.map --
 *	array of sources to try in order. each number is a bitmask:
 *	- lower 4 bits is source type
 *	- upper 4 bits is action bitmap
 *	If source has already been set, don't add again to array
 */

#define NS_DEFAULTMAP	(NS_FILES | NS_SUCCESS)

#define NS_CB(D,E,F,C)		{ D[E].cb = F; D[E].cb_data = C; }
			
#define NS_FILES_CB(D,F,C)	NS_CB(D, NS_FILES, F, C)

#ifdef HESIOD
#   define NS_DNS_CB(D,F,C)	NS_CB(D, NS_DNS, F, C)
#else
#   define NS_DNS_CB(D,F,C)	NS_CB(D, NS_DNS, NULL, NULL)
#endif

#ifdef YP
#   define NS_NIS_CB(D,F,C)	NS_CB(D, NS_NIS, F, C)
#else
#   define NS_NIS_CB(D,F,C)	NS_CB(D, NS_NIS, NULL, NULL)
#endif

#ifdef NISPLUS
#   define NS_NISPLUS_CB(D,F,C)	NS_CB(D, NS_NISPLUS, F, C)
#else
#   define NS_NISPLUS_CB(D,F,C)	NS_CB(D, NS_NISPLUS, NULL, NULL)
#endif

#if defined(HESIOD) || defined(YP) || defined(NISPLUS)
#   define NS_COMPAT_CB(D,F,C)	NS_CB(D, NS_COMPAT, F, C)
#else
#   define NS_COMPAT_CB(D,F,C)	NS_CB(D, NS_COMPAT, NULL, NULL)
#endif


#include <sys/cdefs.h>

__BEGIN_DECLS
extern	void	_nsgetdbt	__P((const char *, ns_DBT *));
extern	void	_nsdumpdbt	__P((const ns_DBT *));
extern	int	nsdispatch	__P((void *, ns_dtab, const char *, ...));
__END_DECLS

#endif /* !_NSSWITCH_H */
