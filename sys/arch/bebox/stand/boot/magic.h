/*	$NetBSD: magic.h,v 1.2 1999/02/15 04:38:06 sakamoto Exp $	*/

char magic[] = "BSD";
int kern_len;
#define	MAGICSIZE	sizeof (magic)
#define	KERNLENSIZE	sizeof (kern_len)
