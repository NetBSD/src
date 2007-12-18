/*	$NetBSD: prep_magic.h,v 1.1 2007/12/18 18:19:07 garbled Exp $	*/

char magic[] = "KMA";
int kern_len;
#define	MAGICSIZE	sizeof (magic)
#define	KERNLENSIZE	sizeof (kern_len)
#define	ENTRY		0x100000
