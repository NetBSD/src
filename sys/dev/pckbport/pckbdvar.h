/* $NetBSD: pckbdvar.h,v 1.2.4.4 2004/09/21 13:32:19 skrll Exp $ */

#include <dev/pckbport/pckbportvar.h>

int	pckbd_cnattach(pckbport_tag_t, int);
void	pckbd_hookup_bell(void (*fn)(void *, u_int, u_int, u_int, int), void *);
