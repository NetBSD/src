/*	$NetBSD: findword.c,v 1.1.1.1.2.2 2012/04/17 00:03:16 yamt Exp $	*/

/*
 * Copyright (C) 2007 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: findword.c,v 1.3 2007/10/25 12:55:32 marttikuparinen Exp 
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
