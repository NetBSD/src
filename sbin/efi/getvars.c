/* $NetBSD: getvars.c,v 1.2 2025/03/02 00:03:41 riastradh Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: getvars.c,v 1.2 2025/03/02 00:03:41 riastradh Exp $");
#endif /* not lint */

#include <sys/efiio.h>
#include <sys/queue.h>

#include <assert.h>
#include <err.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "defs.h"
#include "efiio.h"
#include "getvars.h"
#include "utils.h"

typedef SLIST_HEAD(efi_var_head, efi_var_elm) efi_var_head_t;

typedef struct efi_var_elm {
	efi_var_t	v;
	SLIST_ENTRY(efi_var_elm) entry;
} efi_var_elm_t;

typedef efi_var_head_t getvars_hdl_t;

struct fn_args {
	efi_var_head_t *list_head;
	regex_t preg;
	char *name;
};

/****************************************/
static void
efi_var_cpy(efi_var_ioc_t *dst, efi_var_ioc_t *src)
{

	dst->name     = memdup(src->name, src->namesize);
	dst->namesize = src->namesize;
	dst->vendor   = src->vendor;
	dst->attrib   = src->attrib;

	/* do not dup the data buffer */
	dst->data     = src->data;
	dst->datasize = src->datasize;
	src->data = NULL;
	src->datasize = 0;
}

/****************************************/

static int
var_name_cmp(const void *a, const void *b)
{
	efi_var_t *x = *(efi_var_t * const *)a;
	efi_var_t *y = *(efi_var_t * const *)b;
	int rv;

	rv = strcmp(x->name, y->name);
	if (rv != 0)
		return rv;

	return memcmp(&y->ev.vendor, &x->ev.vendor, sizeof(x->ev.vendor));
}

/****************************************/

static int
save_variable(efi_var_ioc_t *ev, void *vp)
{
	struct fn_args *args = vp;
	efi_var_head_t *head = args->list_head;
	efi_var_elm_t *elm;

	assert(args->name != NULL);

	elm = ecalloc(sizeof(*elm), 1);
	efi_var_cpy(&elm->v.ev, ev);

	elm->v.name = args->name;

	args->name = NULL;

	SLIST_INSERT_HEAD(head, elm, entry);

	return 0;
}

static bool
choose_variable(efi_var_ioc_t *ev, void *vp)
{
	struct fn_args *args = vp;
	bool rv;

	args->name = ucs2_to_utf8(ev->name, ev->namesize, NULL, NULL);

	rv = !regexec(&args->preg, args->name, 0, NULL, 0);
	if (rv == false) {
		free(args->name);
		args->name = NULL;
	}
	return rv;
}

PUBLIC void *
get_variables(int fd, const char *regexp, efi_var_t ***array_ptr,
    size_t *array_cnt)
{
	static efi_var_head_t list_head;
	efi_var_elm_t *elm;
	efi_var_t **var_array;
	size_t var_cnt;
	int i;
	struct fn_args args;

	assert(SLIST_EMPTY(&list_head));

	SLIST_INIT(&list_head);

	memset(&args, 0, sizeof(args));
	args.list_head = &list_head;
	if (regcomp(&args.preg, regexp, REG_EXTENDED) != 0)
		err(EXIT_FAILURE, "regcomp: %s", regexp);

	var_cnt = get_variable_info(fd, choose_variable, save_variable, &args);

	regfree(&args.preg);

	var_array = emalloc(var_cnt * sizeof(*var_array));
	i = 0;
	SLIST_FOREACH(elm, &list_head, entry) {
		var_array[i++] = &elm->v;
	}
	qsort(var_array, var_cnt, sizeof(*var_array), var_name_cmp);

	*array_ptr = var_array;
	*array_cnt = var_cnt;

	return &list_head;
}

static void
free_efi_var_ioc(efi_var_ioc_t *ev)
{

	free(ev->name);
	free(ev->data);
	memset(ev, 0, sizeof(*ev));
}

static void
free_efi_var(efi_var_t *v)
{

	free(v->name);
	memset(v, 0, sizeof(*v));
}

PUBLIC void
free_variables(void *vp)
{
	efi_var_head_t *list_head = vp;
	efi_var_elm_t *elm;

	while (!SLIST_EMPTY(list_head)) {
		elm = SLIST_FIRST(list_head);
		SLIST_REMOVE_HEAD(list_head, entry);
		free_efi_var_ioc(&elm->v.ev);
		free_efi_var(&elm->v);
		free(elm);
	}
}
