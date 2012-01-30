/*	$NetBSD$	*/

/*
 * Copyright (C) 2007 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: findword.c,v 1.3 2007/12/20 09:03:57 darrenr Exp
 */

#include "ipf.h"


wordtab_t *findword(words, name)
	wordtab_t *words;
	char *name;
{
	wordtab_t *w;

	for (w = words; w->w_word != NULL; w++)
		if (!strcmp(name, w->w_word))
			break;
	if (w->w_word == NULL)
		return NULL;

	return w;
}
