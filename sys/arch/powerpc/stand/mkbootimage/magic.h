/*	$NetBSD: magic.h,v 1.3.20.1 2008/06/23 04:30:38 wrstuden Exp $	*/

char bebox_magic[] = "BSD";
char prep_magic[] = "KMA";
char rs6000_magic[] = "KMZ";
int kern_len;

#define BEBOX_MAGICSIZE		sizeof (bebox_magic)
#define	PREP_MAGICSIZE		sizeof (prep_magic)
#define RS6000_MAGICSIZE	sizeof (rs6000_magic)
#define	KERNLENSIZE		sizeof (kern_len)
#define	BEBOX_ENTRY		0x3100
