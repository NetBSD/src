/*	$NetBSD: bootptest.h,v 1.3 1998/03/14 04:39:54 lukem Exp $	*/

/* bootptest.h */
/*
 * Hacks for sharing print-bootp.c between tcpdump and bootptest.
 */
#define ESRC(p) (p)
#define EDST(p) (p)

#ifndef	USE_BFUNCS
/* Use mem/str functions */
/* There are no overlapped copies, so memcpy is OK. */
#define bcopy(a,b,c)    memcpy(b,a,c)
#define bzero(p,l)      memset(p,0,l)
#define bcmp(a,b,c)     memcmp(a,b,c)
#endif

extern int vflag; /* verbose flag */

/* global pointers to beginning and end of current packet (during printing) */
extern unsigned char *packetp;
extern unsigned char *snapend;

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern void bootp_print P((struct bootp *, int, u_short, u_short));
extern char *ipaddr_string P((struct in_addr *));
extern int printfn P((u_char *, u_char *));

#undef P
