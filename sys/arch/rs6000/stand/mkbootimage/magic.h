/*	$NetBSD: magic.h,v 1.1.8.2 2008/01/21 09:39:07 yamt Exp $	*/

char magic[] = "KMZ";
int kern_len;
#define	MAGICSIZE	sizeof (magic)
#define	KERNLENSIZE	sizeof (kern_len)
#define	ENTRY		0x100000
