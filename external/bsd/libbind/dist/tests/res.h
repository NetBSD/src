/*	$NetBSD: res.h,v 1.1.1.1.2.2 2009/05/13 18:52:36 jym Exp $	*/

/*
 * Copyright (C) 2009  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 *	@(#)res.h	5.10 (Berkeley) 6/1/90
 *	Id: res.h,v 1.3 2009/03/03 23:49:07 tbox Exp
 */

/*
 *******************************************************************************
 *
 *  res.h --
 *
 *	Definitions used by modules of the name server lookup program.
 *
 *	Copyright (c) 1985
 *	Andrew Cherenson
 *	U.C. Berkeley
 *	CS298-26  Fall 1985
 * 
 *******************************************************************************
 */

#define TRUE	1
#define FALSE	0
typedef int Boolean;

#define MAXALIASES	35
#define MAXADDRS	35
#define MAXDOMAINS	35
#define MAXSERVERS	10

/*
 *  Define return statuses in addtion to the ones defined in namserv.h
 *   let SUCCESS be a synonym for NOERROR
 *
 *	TIME_OUT	- a socket connection timed out.
 *	NO_INFO		- the server didn't find any info about the host.
 *	ERROR		- one of the following types of errors:
 *			   dn_expand, res_mkquery failed
 *			   bad command line, socket operation failed, etc.
 *	NONAUTH		- the server didn't have the desired info but
 *			  returned the name(s) of some servers who should.
 *	NO_RESPONSE	- the server didn't respond.
 *
 */

#ifdef ERROR
#undef ERROR
#endif

#define  SUCCESS		0
#define  TIME_OUT		-1
#define  NO_INFO		-2
#define  ERROR			-3
#define  NONAUTH		-4
#define  NO_RESPONSE		-5

/*
 *  Define additional options for the resolver state structure.
 *
 *   RES_DEBUG2		more verbose debug level
 */

#define RES_DEBUG2	0x00400000

/*
 *  Maximum length of server, host and file names.
 */

#define NAME_LEN 256


/*
 * Modified struct hostent from <netdb.h>
 *
 * "Structures returned by network data base library.  All addresses
 * are supplied in host order, and returned in network order (suitable
 * for use in system calls)."
 */

typedef struct {
	int     addrType;
	int     addrLen;
	char    *addr;
} AddrInfo;

typedef struct	{
	char	*name;		/* official name of host */
	char	**domains;	/* domains it serves */
	AddrInfo **addrList;	/* list of addresses from name server */
} ServerInfo;

typedef struct	{
	char	*name;		/* official name of host */
	char	**aliases;	/* alias list */
	AddrInfo **addrList;	/* list of addresses from name server */
	ServerInfo **servers;
} HostInfo;

/*
 *  External routines:
 */

extern int pickString(const char *, char *, size_t);
extern int matchString(const char *, const char *);
extern int StringToType(char *, int, FILE *);
extern int StringToClass(char *, int, FILE *);

