/*	$NetBSD: printportcmp.c,v 1.1.1.3 2012/01/30 16:03:23 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: printportcmp.c,v 1.11.2.1 2012/01/26 05:29:16 darrenr Exp
 */

#include "ipf.h"


void
printportcmp(pr, frp)
	int	pr;
	frpcmp_t	*frp;
{
	static char *pcmp1[] = { "*", "=", "!=", "<", ">", "<=", ">=",
				 "<>", "><", ":" };

	if (frp->frp_cmp == FR_INRANGE || frp->frp_cmp == FR_OUTRANGE)
		PRINTF(" port %d %s %d", frp->frp_port,
			     pcmp1[frp->frp_cmp], frp->frp_top);
	else if (frp->frp_cmp == FR_INCRANGE)
		PRINTF(" port %d:%d", frp->frp_port, frp->frp_top);
	else
		PRINTF(" port %s %s", pcmp1[frp->frp_cmp],
			     portname(pr, frp->frp_port));
}
