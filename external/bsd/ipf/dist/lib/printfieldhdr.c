/*	$NetBSD: printfieldhdr.c,v 1.1.1.1.2.2 2012/04/17 00:03:19 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: printfieldhdr.c,v 1.5.2.2 2012/01/26 05:44:26 darren_r Exp 
 */

#include "ipf.h"
#include <ctype.h>


void
printfieldhdr(words, field)
	wordtab_t *words, *field;
{
	wordtab_t *w;
	char *s, *t;
	int i;

	if (field->w_value == -2) {
		for (i = 0, w = words; w->w_word != NULL; ) {
			if (w->w_value > 0) {
				printfieldhdr(words, w);
				w++;
				if (w->w_value > 0)
					putchar('\t');
			} else {
				w++;
			}
		}
		return;
	}

	for (w = words; w->w_word != NULL; w++) {
		if (w->w_value == field->w_value) {
			if (w->w_word == field->w_word) {
				s = strdup(w->w_word);
			} else {
				s = NULL;
			}

			if ((w->w_word != field->w_word) || (s == NULL)) {
				PRINTF("%s", field->w_word);
			} else {
				for (t = s; *t != '\0'; t++) {
					if (ISALPHA(*t) && ISLOWER(*t))
						*t = TOUPPER(*t);
				}
				PRINTF("%s", s);
				free(s);
			}
		}
	}
}
