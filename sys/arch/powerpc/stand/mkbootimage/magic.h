/*	$NetBSD: magic.h,v 1.3.4.2 2008/01/02 21:49:15 bouyer Exp $	*/

char bebox_magic[] = "BSD";
char prep_magic[] = "KMA";
char rs6000_magic[] = "KMZ";
int kern_len;

#define BEBOX_MAGICSIZE		sizeof (bebox_magic)
#define	PREP_MAGICSIZE		sizeof (prep_magic)
#define RS6000_MAGICSIZE	sizeof (rs6000_magic)
#define	KERNLENSIZE		sizeof (kern_len)
#define	ENTRY			0x100000
