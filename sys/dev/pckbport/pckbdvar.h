/* $NetBSD: pckbdvar.h,v 1.1 2004/03/13 17:31:33 bjh21 Exp $ */

#include <dev/pckbport/pckbportvar.h>

int	pckbd_cnattach __P((pckbport_tag_t, int));
void	pckbd_hookup_bell __P((void (*fn)(void *, u_int, u_int, u_int, int),
	    void *));
