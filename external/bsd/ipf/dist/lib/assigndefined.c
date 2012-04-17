/*	$NetBSD: assigndefined.c,v 1.1.1.1.2.2 2012/04/17 00:03:16 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: assigndefined.c,v 1.4.2.1 2012/01/26 05:44:26 darren_r Exp 
 */

#include "ipf.h"

void assigndefined(env)
	char *env;
{
	char *s, *t;

	if (env == NULL)
		return;

	for (s = strtok(env, ";"); s != NULL; s = strtok(NULL, ";")) {
		t = strchr(s, '=');
		if (t == NULL)
			continue;
		*t++ = '\0';
		set_variable(s, t);
		*--t = '=';
	}
}
