/* $NetBSD: config.c,v 1.2 2003/03/04 19:28:59 jmmv Exp $ */

/*
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio Merino.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name authors may not be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: config.c,v 1.2 2003/03/04 19:28:59 jmmv Exp $");
#endif /* not lint */

#include <sys/time.h>
#include <dev/wscons/wsconsio.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <string.h>

#include "pathnames.h"
#include "wsmoused.h"

static struct block *Global = NULL;

/* Prototypes for config_yacc.y (only used here) */
struct block *config_parse(FILE *);

/*
 * Creates a new, empty property.  Returns a pointer to it.
 */
struct prop *
prop_new(void)
{
	struct prop *p;

	p = (struct prop *) calloc(1, sizeof(struct prop));
	if (p == NULL)
		err(EXIT_FAILURE, "calloc");
	return p;
}

/*
 * Frees a property created with prop_new.  All data stored in the property
 * is also destroyed.
 */
void
prop_free(struct prop *p)
{
	free(p->p_name);
	free(p->p_value);
	free(p);
}

/*
 * Creates a new, empty block, with the specified type (see BLOCK_* macros).
 * Returns a pointer to it.
 */
struct block *
block_new(int type)
{
	struct block *b;

	b = (struct block *) calloc(1, sizeof(struct block));
	if (b == NULL)
		err(EXIT_FAILURE, "calloc");
	b->b_type = type;
	return b;
}

/*
 * Frees a block created with block_new.  All data contained inside the block
 * is also destroyed.
 */
void
block_free(struct block *b)
{
	int i;

	if (b->b_name != NULL)
		free(b->b_name);

	for (i = 0; i < b->b_prop_count; i++)
		prop_free(b->b_prop[i]);
	for (i = 0; i < b->b_child_count; i++)
		block_free(b->b_child[i]);

	free(b);
}

/*
 * Add a property to a block.
 */
void
block_add_prop(struct block *b, struct prop *p)
{
	if (p == NULL)
		return;

	if (b->b_prop_count >= MAX_PROPS)
		errx(EXIT_FAILURE, "too many properties for current block");
	else {
		b->b_prop[b->b_prop_count] = p;
		b->b_prop_count++;
	}
}

/*
 * Add a child (block) to a block.
 */
void
block_add_child(struct block *b, struct block *c)
{
	if (c == NULL)
		return;

	if (b->b_child_count >= MAX_BLOCKS)
		errx(EXIT_FAILURE, "too many childs for current block");
	else {
		c->b_parent = b;
		b->b_child[b->b_child_count] = c;
		b->b_child_count++;
	}
}

/*
 * Get the value of a property in the specified block (or in its parents).
 * If not found, return the value given in def.
 */
char *
block_get_propval(struct block *b, char *pname, char *def)
{
	int pc;

	if (b == NULL)
		return def;

	while (b != NULL) {
		for (pc = 0; pc < b->b_prop_count; pc++)
			if (strcmp(b->b_prop[pc]->p_name, pname) == 0)
				return b->b_prop[pc]->p_value;
		b = b->b_parent;
	}

	return def;
}

/*
 * Get the value of a property in the specified block converting it to an
 * integer, if possible.  If the property cannot be found in the given
 * block, all its parents are tried.  If after all not found (or conversion
 * not possible), return the value given in def.
 */
int
block_get_propval_int(struct block *b, char *pname, int def)
{
	int pc, ret;
	char *ptr;

	if (b == NULL)
		return def;

	while (b != NULL) {
		for (pc = 0; pc < b->b_prop_count; pc++)
			if (strcmp(b->b_prop[pc]->p_name, pname) == 0) {
				ret = (int) strtol(b->b_prop[pc]->p_value,
				    &ptr, 10);
				if (b->b_prop[pc]->p_value == ptr) {
					warnx("expected integer in `%s' "
					    "property", pname);
					return def;
				}
				return ret;
			}
		b = b->b_parent;
	}

	return def;
}

/*
 * Get a mode block (childs of the global scope), which matches the specified
 * name.
 */
struct block *
config_get_mode(char *modename)
{
	struct block *b = Global;
	int bc;

	if (b != NULL)
		for (bc = 0; bc < b->b_child_count; bc++)
			if (strcmp(b->b_child[bc]->b_name, modename) == 0)
				return b->b_child[bc];

	return NULL;
}

/*
 * Read the configuration file.
 */
void
config_read(char *conffile, int opt)
{
	FILE *f;

	errno = 0;
	f = fopen(conffile, "r");
	if (f != NULL) {
		Global = config_parse(f);
		if (Global == NULL)
			errx(EXIT_FAILURE, "%s contains fatal errors",
			     conffile);
	} else if (errno != ENOENT || opt) {
		err(EXIT_FAILURE, "cannot open %s", conffile);
	}
}

/*
 * Destroy all the configuration data.
 */
void
config_free(void)
{
	if (Global != NULL)
		block_free(Global);
}
