/* $NetBSD: pckbdvar.h,v 1.2 2004/03/18 21:05:19 bjh21 Exp $ */

#include <dev/pckbport/pckbportvar.h>

int	pckbd_cnattach(pckbport_tag_t, int);
void	pckbd_hookup_bell(void (*fn)(void *, u_int, u_int, u_int, int), void *);
