/* $NetBSD: pckbdvar.h,v 1.1.14.1 2000/11/20 11:42:40 bouyer Exp $ */

int	pckbd_cnattach __P((pckbc_tag_t, int));
void	pckbd_hookup_bell __P((void (*fn)(void *, u_int, u_int, u_int, int),
	    void *));
