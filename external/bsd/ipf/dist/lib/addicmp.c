/*	$NetBSD: addicmp.c,v 1.2 2012/07/22 14:27:36 darrenr Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: addicmp.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $
 */

#include <ctype.h>

#include "ipf.h"


char	*icmptypes[MAX_ICMPTYPE + 1] = {
	"echorep", (char *)NULL, (char *)NULL, "unreach", "squench",
	"redir", (char *)NULL, (char *)NULL, "echo", "routerad",
	"routersol", "timex", "paramprob", "timest", "timestrep",
	"inforeq", "inforep", "maskreq", "maskrep", "END"
};
