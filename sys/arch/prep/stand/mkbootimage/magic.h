/*	$NetBSD: magic.h,v 1.1 2000/02/29 15:21:52 nonaka Exp $	*/

char magic[] = "KMA";
int kern_len;
#define	MAGICSIZE	sizeof (magic)
#define	KERNLENSIZE	sizeof (kern_len)
#define	ENTRY		0x100000
