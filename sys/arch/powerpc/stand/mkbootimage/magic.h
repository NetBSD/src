/*	$NetBSD: magic.h,v 1.2 2007/12/19 19:46:31 garbled Exp $	*/

char prep_magic[] = "KMA";
char rs6000_magic[] = "KMZ";
int kern_len;
#define	PREP_MAGICSIZE		sizeof (prep_magic)
#define RS6000_MAGICSIZE	sizeof (rs6000_magic)
#define	KERNLENSIZE		sizeof (kern_len)
#define	ENTRY			0x100000
