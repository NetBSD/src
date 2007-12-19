/*	$NetBSD: magic.h,v 1.1 2007/12/19 19:45:32 garbled Exp $	*/

char prep_magic[] = "KMA";
int kern_len;
#define	PREP_MAGICSIZE	sizeof (prep_magic)
#define	KERNLENSIZE	sizeof (kern_len)
#define	ENTRY		0x100000
