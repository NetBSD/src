/* $NetBSD: utils.c,v 1.1 2002/10/04 18:37:21 elric Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland C. Dowdeswell.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: utils.c,v 1.1 2002/10/04 18:37:21 elric Exp $");
#endif

#include <malloc.h>
#include <string.h>

#include "utils.h"

/* just strsep(3), but skips empty fields. */

static char *
strsep_getnext(char **stringp, const char *delim)
{
	char	*ret;

	ret = strsep(stringp, delim);
	while (ret && index(delim, *ret))
		ret = strsep(stringp, delim);
	return ret;
}

/*
 * this function returns a dynamically sized char ** of the words
 * in the line.  the caller is responsible for both free(3)ing
 * each word and the superstructure by calling words_free().
 */
char **
words(const char *line, int *num)
{
	int	  i = 0;
	int	  nwords = 0;
	char	 *cur;
	char	**ret;
	char	 *tmp;
	char	 *tmp1;

	*num = 0;
	tmp = (char *)line;
	if (tmp[0] == '\0')
		return NULL;
	while (tmp[0]) {
		if ((tmp[1] == ' ' || tmp[1] == '\t' || tmp[1] == '\0') &&
		    (tmp[0] != ' ' && tmp[0] != '\t'))
			nwords++;
		tmp++;
	}
	ret = malloc((nwords+1) * sizeof(char *));
	tmp1 = tmp = strdup(line);
	while ((cur = strsep_getnext(&tmp, " \t")) != NULL)
		ret[i++] = strdup(cur);
	ret[i] = NULL;
	free(tmp1);
	*num = nwords;
	return ret;
}

void
words_free(char **w, int num)
{
	int	i;

	for (i=0; i < num; i++)
		free(w[i]);
}

/*
 * this is a simple xor that has the same calling conventions as
 * memcpy(3).
 */

void
memxor(void *res, const void *src, size_t len)
{
	int i;
	char *r;
	const char *s;

	r = res;
	s = src;
	for (i=0; i < len; i++)
		r[i] ^= s[i];
}
