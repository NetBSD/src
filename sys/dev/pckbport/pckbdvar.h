/* $NetBSD: pckbdvar.h,v 1.2.18.1 2008/01/21 09:44:26 yamt Exp $ */

#include <dev/pckbport/pckbportvar.h>

int	pckbd_cnattach(pckbport_tag_t, int);
void	pckbd_hookup_bell(void (*fn)(void *, u_int, u_int, u_int, int), void *);
void	pckbd_unhook_bell(void (*fn)(void *, u_int, u_int, u_int, int), void *);
