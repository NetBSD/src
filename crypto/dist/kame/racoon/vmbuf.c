/*	$KAME: vmbuf.c,v 1.11 2001/11/26 16:54:29 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: vmbuf.c,v 1.2 2003/07/12 09:37:13 itojun Exp $");

#define NONEED_DRM
#include <sys/types.h>
#include <sys/param.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "debug.h"
#include "gcmalloc.h"

vchar_t *
vmalloc(size)
	size_t size;
{
	vchar_t *var;

	if ((var = (vchar_t *)racoon_malloc(sizeof(*var))) == NULL)
		return NULL;

	var->l = size;
	var->v = (caddr_t)racoon_calloc(1, size);
	if (var->v == NULL) {
		(void)racoon_free(var);
		return NULL;
	}

	return var;
}

vchar_t *
vrealloc(ptr, size)
	vchar_t *ptr;
	size_t size;
{
	caddr_t v;

	if (ptr != NULL) {
		if ((v = (caddr_t)racoon_realloc(ptr->v, size)) == NULL) {
			(void)vfree(ptr);
			return NULL;
		}
		memset(v + ptr->l, 0, size - ptr->l);
		ptr->v = v;
		ptr->l = size;
	} else {
		if ((ptr = vmalloc(size)) == NULL)
			return NULL;
	}

	return ptr;
}

void
vfree(var)
	vchar_t *var;
{
	if (var == NULL)
		return;

	if (var->v)
		(void)racoon_free(var->v);

	(void)racoon_free(var);

	return;
}

vchar_t *
vdup(src)
	vchar_t *src;
{
	vchar_t *new;

	if ((new = vmalloc(src->l)) == NULL)
		return NULL;

	memcpy(new->v, src->v, src->l);

	return new;
}
