/*	$NetBSD: strlist.c,v 1.1 2008/12/20 22:41:53 dsl Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Laight.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

#ifndef MAKE_NATIVE
static char rcsid[] = "$NetBSD: strlist.c,v 1.1 2008/12/20 22:41:53 dsl Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: strlist.c,v 1.1 2008/12/20 22:41:53 dsl Exp $");
#endif /* not lint */
#endif

#include <stddef.h>
#include <stdlib.h>
#include "strlist.h"
#include "make.h"

void
strlist_init(strlist_t *sl)
{
	sl->sl_num = 0;
	sl->sl_str = NULL;
}

void
strlist_clean(strlist_t *sl)
{
	char *str;
	int i;

	STRLIST_FOREACH(str, sl, i)
		free(str);
	free(sl->sl_str);

	sl->sl_num = 0;
	sl->sl_str = NULL;
}

void
strlist_add_str(strlist_t *sl, char *str)
{
	unsigned int n;
	char **new_strs;

	if (str == NULL)
	    return;

	n = sl->sl_num + 1;
	sl->sl_num = n;
	new_strs = bmake_realloc(sl->sl_str, (n + 1) * sizeof *sl->sl_str);
	new_strs[n - 1] = str;
	new_strs[n] = NULL;         /* STRLIST_FOREACH() terminator */
	sl->sl_str = new_strs;
}
