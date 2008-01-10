/* $NetBSD: pckbdvar.h,v 1.3.60.1 2008/01/10 23:44:25 bouyer Exp $ */

#include <dev/pckbport/pckbportvar.h>

int	pckbd_cnattach(pckbport_tag_t, int);
void	pckbd_hookup_bell(void (*fn)(void *, u_int, u_int, u_int, int), void *);
void	pckbd_unhook_bell(void (*fn)(void *, u_int, u_int, u_int, int), void *);
