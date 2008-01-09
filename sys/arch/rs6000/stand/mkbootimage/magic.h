/*	$NetBSD: magic.h,v 1.1.6.2 2008/01/09 01:48:34 matt Exp $	*/

char magic[] = "KMZ";
int kern_len;
#define	MAGICSIZE	sizeof (magic)
#define	KERNLENSIZE	sizeof (kern_len)
#define	ENTRY		0x100000
