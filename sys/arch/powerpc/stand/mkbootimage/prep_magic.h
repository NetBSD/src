/*	$NetBSD: prep_magic.h,v 1.2 2007/12/18 18:26:36 garbled Exp $	*/

char prep_magic[] = "KMA";
int kern_len;
#define	PREP_MAGICSIZE	sizeof (prep_magic)
#define	KERNLENSIZE	sizeof (kern_len)
#define	ENTRY		0x100000
