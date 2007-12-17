/*	$NetBSD: magic.h,v 1.1.2.2 2007/12/17 19:09:58 garbled Exp $	*/

char magic[] = "KMZ";
int kern_len;
#define	MAGICSIZE	sizeof (magic)
#define	KERNLENSIZE	sizeof (kern_len)
#define	ENTRY		0x100000
