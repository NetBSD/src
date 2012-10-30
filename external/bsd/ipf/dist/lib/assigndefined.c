/*	$NetBSD: assigndefined.c,v 1.1.1.1.2.3 2012/10/30 18:55:03 yamt Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: assigndefined.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $
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
