/* $NetBSD: pckbdvar.h,v 1.2 2000/03/06 21:40:08 thorpej Exp $ */

int	pckbd_cnattach __P((pckbc_tag_t, int));
void	pckbd_hookup_bell __P((void (*fn)(void *, u_int, u_int, u_int),
	    void *));
