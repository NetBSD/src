/*	$NetBSD: magic.h,v 1.4 1999/06/28 01:03:55 sakamoto Exp $	*/

char magic[] = "BSD";
int kern_len;
#define	MAGICSIZE	sizeof (magic)
#define	KERNLENSIZE	sizeof (kern_len)
#define	ENTRY		0x3100
