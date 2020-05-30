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

/*
 * NPF variables are used to build the intermediate representation (IR)
 * of the configuration grammar.  They represent primitive types (strings,
 * numbers, etc) as well as complex types (address and mask, table, etc).
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_var.c,v 1.13 2020/05/30 14:16:56 rmind Exp $");

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define _NPFVAR_PRIVATE
#include "npfctl.h"

typedef struct npf_element {
	void *		e_data;
	unsigned	e_type;
	struct npf_element *e_next;
} npf_element_t;

struct npfvar {
	char *		v_key;
	npf_element_t *	v_elements;
	npf_element_t *	v_last;
	size_t		v_count;
	void *		v_next;
};

static npfvar_t *	var_list = NULL;
static size_t		var_num = 0;

npfvar_t *
npfvar_create(void)
{
	npfvar_t *vp = ecalloc(1, sizeof(*vp));
	var_num++;
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
npfvar_add(npfvar_t *vp, const char *name)
{
	vp->v_key = estrdup(name);
	vp->v_next = var_list;
	var_list = vp;
}

npfvar_t *
npfvar_create_element(unsigned type, const void *data, size_t len)
{
	npfvar_t *vp = npfvar_create();
	return npfvar_add_element(vp, type, data, len);
}

npfvar_t *
npfvar_create_from_string(unsigned type, const char *string)
{
	return npfvar_create_element(type, string, strlen(string) + 1);
}

npfvar_t *
npfvar_add_element(npfvar_t *vp, unsigned type, const void *data, size_t len)
{
	npf_element_t *el;

	el = ecalloc(1, sizeof(*el));
	el->e_data = ecalloc(1, len);
	el->e_type = type;
	memcpy(el->e_data, data, len);

	/* Preserve the order of insertion. */
	if (vp->v_elements == NULL) {
		vp->v_elements = el;
	} else {
		vp->v_last->e_next = el;
	}
	vp->v_last = el;
	vp->v_count++;
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
			vp->v_elements = vp2->v_elements;
		}
	} else if (vp2->v_elements) {
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
	var_num--;
}

char *
npfvar_expand_string(const npfvar_t *vp)
{
	if (npfvar_get_count(vp) != 1) {
		yyerror("variable '%s' has multiple elements", vp->v_key);
		return NULL;
	}
	return npfvar_get_data(vp, NPFVAR_STRING, 0);
}

size_t
npfvar_get_count(const npfvar_t *vp)
{
	return vp ? vp->v_count : 0;
}

static npf_element_t *
npfvar_get_element(const npfvar_t *vp, size_t idx, size_t level)
{
	npf_element_t *el;

	/*
	 * Verify the parameters.
	 */
	if (vp == NULL) {
		return NULL;
	}
	if (level >= var_num) {
		yyerror("circular dependency for variable '%s'", vp->v_key);
		return NULL;
	}
	if (vp->v_count <= idx) {
		yyerror("variable '%s' has only %zu elements, requested %zu",
		    vp->v_key, vp->v_count, idx);
		return NULL;
	}

	/*
	 * Get the element at the given index.
	 */
	el = vp->v_elements;
	while (idx--) {
		el = el->e_next;
	}

	/*
	 * Resolve if it is a reference to another variable.
	 */
	if (el->e_type == NPFVAR_VAR_ID) {
		const npfvar_t *rvp = npfvar_lookup(el->e_data);
		return npfvar_get_element(rvp, 0, level + 1);
	}
	return el;
}

int
npfvar_get_type(const npfvar_t *vp, size_t idx)
{
	npf_element_t *el = npfvar_get_element(vp, idx, 0);
	return el ? (int)el->e_type : -1;
}

void *
npfvar_get_data(const npfvar_t *vp, unsigned type, size_t idx)
{
	npf_element_t *el = npfvar_get_element(vp, idx, 0);

	if (el && NPFVAR_TYPE(el->e_type) != NPFVAR_TYPE(type)) {
		yyerror("variable '%s' element %zu "
		    "is of type '%s' rather than '%s'", vp->v_key,
		    idx, npfvar_type(el->e_type), npfvar_type(type));
		return NULL;
	}
	return el->e_data;
}
