/*	$Id: magic.h,v 1.1 1998/01/16 04:17:55 sakamoto Exp $	*/

char magic[] = "BSD";
int kern_len;
#define	MAGICSIZE	sizeof (magic)
#define	KERNLENSIZE	sizeof (kern_len)
