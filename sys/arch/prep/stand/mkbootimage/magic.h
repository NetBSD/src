/*	$NetBSD: magic.h,v 1.1.6.2 2000/11/20 20:23:11 bouyer Exp $	*/

char magic[] = "KMA";
int kern_len;
#define	MAGICSIZE	sizeof (magic)
#define	KERNLENSIZE	sizeof (kern_len)
#define	ENTRY		0x100000
