/*	$NetBSD: magic.h,v 1.1.4.2 2008/01/02 21:50:12 bouyer Exp $	*/

char magic[] = "KMZ";
int kern_len;
#define	MAGICSIZE	sizeof (magic)
#define	KERNLENSIZE	sizeof (kern_len)
#define	ENTRY		0x100000
