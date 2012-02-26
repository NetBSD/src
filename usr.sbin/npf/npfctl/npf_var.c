/*	$NetBSD: npf_var.c,v 1.4 2012/02/26 21:50:05 christos Exp $	*/

/*-
 * Copyright (c) 2011-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
__RCSID("$NetBSD: npf_var.c,v 1.4 2012/02/26 21:50:05 christos Exp $");

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define _NPFVAR_PRIVATE
#include "npfctl.h"

typedef struct npf_element {
	void *		e_data;
	int		e_type;
	struct npf_element *e_next;
} npf_element_t;

struct npfvar {
	char *		v_key;
	npf_element_t *	v_elements;
	npf_element_t *	v_last;
	int		v_type;
	size_t		v_count;
	void *		v_next;
};

static npfvar_t *	var_list = NULL;
static size_t		var_num = 0;

npfvar_t *
npfvar_create(const char *name)
{
	npfvar_t *vp = zalloc(sizeof(*vp));
	vp->v_key = xstrdup(name);
	return vp;
}

npfvar_t *
npfvar_lookup(const char *key)
{
	for (npfvar_t *it = var_list; it != NULL; it = it->v_next)
		if (strcmp(it->v_key, key) == 0)
			return it;
	return NULL;
}

const char *
npfvar_type(size_t t)
{
	if (t >= __arraycount(npfvar_types)) {
		return "unknown";
	}
	return npfvar_types[t];
}

void
npfvar_add(npfvar_t *vp)
{
	vp->v_next = var_list;
	var_list = vp;
	var_num++;
}

npfvar_t *
npfvar_add_element(npfvar_t *vp, int type, const void *data, size_t len)
{
	npf_element_t *el;

	if (vp->v_count == 0) {
		vp->v_type = type;
	} else if (NPFVAR_TYPE(vp->v_type) != NPFVAR_TYPE(type)) {
		yyerror("element type '%s' does not match variable type '%s'",
		    npfvar_type(type), npfvar_type(vp->v_type));
		return NULL;
	}
	vp->v_count++;
	el = zalloc(sizeof(*el));
	el->e_data = zalloc(len);
	el->e_type = type;
	memcpy(el->e_data, data, len);

	/* Preserve order of insertion. */
	if (vp->v_elements == NULL) {
		vp->v_elements = el;
	} else {
		vp->v_last->e_next = el;
	}
	vp->v_last = el;
	return vp;
}

npfvar_t *
npfvar_add_elements(npfvar_t *vp, npfvar_t *vp2)
{
	if (vp2 == NULL)
		return vp;
	if (vp == NULL)
		return vp2;

	if (vp->v_elements == NULL) {
		if (vp2->v_elements) {
			vp->v_type = vp2->v_type;
			vp->v_elements = vp2->v_elements;
		}
	} else if (vp2->v_elements) {
		if (NPFVAR_TYPE(vp->v_type) != NPFVAR_TYPE(vp2->v_type)) {
			yyerror("variable '%s' type '%s' does not match "
			    "variable '%s' type '%s'", vp->v_key,
			    npfvar_type(vp->v_type),
			    vp2->v_key, npfvar_type(vp2->v_type));
			return NULL;
		}
		vp->v_last->e_next = vp2->v_elements;
	}
	if (vp2->v_elements) {
		vp->v_last = vp2->v_last;
		vp->v_count += vp2->v_count;
		vp2->v_elements = NULL;
		vp2->v_count = 0;
		vp2->v_last = NULL;
	}
	npfvar_destroy(vp2);
	return vp;
}

static void
npfvar_free_elements(npf_element_t *el)
{
	if (el == NULL)
		return;
	npfvar_free_elements(el->e_next);
	free(el->e_data);
	free(el);
}

void
npfvar_destroy(npfvar_t *vp)
{
	npfvar_free_elements(vp->v_elements);
	free(vp->v_key);
	free(vp);
}

char *
npfvar_expand_string(const npfvar_t *vp)
{
	return npfvar_get_data(vp, NPFVAR_STRING, 0);
}

size_t
npfvar_get_count(const npfvar_t *vp)
{
	return vp ? vp->v_count : 0;
}

static void *
npfvar_get_data1(const npfvar_t *vp, int type, size_t idx, size_t level)
{
	npf_element_t *el;

	if (level >= var_num) {
		yyerror("variable loop for '%s'", vp->v_key);
		return NULL;
	}

	if (vp == NULL)
		return NULL;

	if (NPFVAR_TYPE(vp->v_type) != NPFVAR_TYPE(type)) {
		yyerror("variable '%s' is of type '%s' not '%s'", vp->v_key,
		    npfvar_type(vp->v_type), npfvar_type(type));
		return NULL;
	}

	if (vp->v_count <= idx) {
		yyerror("variable '%s' has only %zu elements, requested %zu",
		    vp->v_key, vp->v_count, idx);
		return NULL;
	}

	el = vp->v_elements;
	while (idx--) {
		el = el->e_next;
	}

	if (vp->v_type == NPFVAR_VAR_ID) {
		npfvar_t *rvp = npfvar_lookup(el->e_data);
		return npfvar_get_data1(rvp, type, 0, level + 1);
	}
	return el->e_data;
}

static int
npfvar_get_type1(const npfvar_t *vp, size_t idx, size_t level)
{
	npf_element_t *el;

	if (level >= var_num) {
		yyerror("variable loop for '%s'", vp->v_key);
		return -1;
	}

	if (vp == NULL)
		return -1;

	if (vp->v_count <= idx) {
		yyerror("variable '%s' has only %zu elements, requested %zu",
		    vp->v_key, vp->v_count, idx);
		return -1;
	}

	el = vp->v_elements;
	while (idx--) {
		el = el->e_next;
	}

	if (vp->v_type == NPFVAR_VAR_ID) {
		npfvar_t *rvp = npfvar_lookup(el->e_data);
		return npfvar_get_type1(rvp, 0, level + 1);
	}
	return el->e_type;
}

int
npfvar_get_type(const npfvar_t *vp, size_t idx)
{
	return npfvar_get_type1(vp, idx, 0);
}

void *
npfvar_get_data(const npfvar_t *vp, int type, size_t idx)
{
	return npfvar_get_data1(vp, type, idx, 0);
}
