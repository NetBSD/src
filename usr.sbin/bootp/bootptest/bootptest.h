/*	$NetBSD: bootptest.h,v 1.5 2007/05/27 16:31:42 tls Exp $	*/

/* bootptest.h */
/*
 * Hacks for sharing print-bootp.c between tcpdump and bootptest.
 */
#define ESRC(p) (p)
#define EDST(p) (p)

extern int vflag; /* verbose flag */

/* global pointers to beginning and end of current packet (during printing) */
extern unsigned char *packetp;
extern unsigned char *snapend;

extern void bootp_print(struct bootp *, int, u_short, u_short);
extern char *ipaddr_string(struct in_addr *);
extern int printfn(u_char *, u_char *);
