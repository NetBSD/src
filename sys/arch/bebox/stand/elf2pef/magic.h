/*	$NetBSD: magic.h,v 1.3 1999/06/28 00:56:03 sakamoto Exp $	*/

char magic[] = "BSD";
int kern_len;
#define	MAGICSIZE	sizeof (magic)
#define	KERNLENSIZE	sizeof (kern_len)
