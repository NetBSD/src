/*
 * Copyright (c) 1994 Christos Zoulas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$NetBSD: stringlist.c,v 1.1.2.1 1996/11/06 00:48:41 lukem Exp $";
#endif /* LIBC_SCCS and not lint */

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <stdlib.h>
#include <stringlist.h>

/*
 * _sl_init(): Initialize a string list
 */
struct stringlist *
_sl_init()
{
	struct stringlist *sl = malloc(sizeof(struct stringlist));
	if (sl == NULL)
		_err(1, "stringlist: %m");

	sl->sl_cur = 0;
	sl->sl_max = 20;
	sl->sl_str = malloc(sl->sl_max * sizeof(char *));
	if (sl->sl_str == NULL)
		_err(1, "stringlist: %m");
	return sl;
}


/*
 * _sl_add(): Add an item to the string list
 */
void
_sl_add(sl, name)
	struct stringlist	*sl;
	char			*name;
{
	if (sl->sl_cur == sl->sl_max - 1) {
		sl->sl_max += 20;
		sl->sl_str = realloc(sl->sl_str, sl->sl_max * sizeof(char *));
		if (sl->sl_str == NULL)
			_err(1, "stringlist: %m");
	}
	sl->sl_str[sl->sl_cur++] = name;
}


/*
 * _sl_free(): Free a stringlist
 */
void
_sl_free(sl, all)
	struct stringlist	*sl;
	int		 	all;
{
	size_t	i;

	if (sl == NULL)
		return;
	if (sl->sl_str) {
		if (all)
			for (i = 0; i < sl->sl_cur; i++)
				free(sl->sl_str[i]);
		free(sl->sl_str);
	}
	free(sl);
}


/*
 * sl_find(): Find a name in the string list
 */
char *
_sl_find(sl, name)
	struct stringlist	*sl;
	char			*name;
{
	size_t	i;

	for (i = 0; i < sl->sl_cur; i++)
		if (strcmp(sl->sl_str[i], name) == 0)
			return sl->sl_str[i];

	return NULL;
}
