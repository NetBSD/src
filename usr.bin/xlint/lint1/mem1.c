/*	$NetBSD: mem1.c,v 1.71 2023/07/15 15:56:17 rillig Exp $	*/

/*
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
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
 *	This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID)
__RCSID("$NetBSD: mem1.c,v 1.71 2023/07/15 15:56:17 rillig Exp $");
#endif

#include <sys/param.h>
#include <stdlib.h>
#include <string.h>

#include "lint1.h"

/*
 * Filenames allocated by record_filename are shared and have unlimited
 * lifetime.
 */
struct filename {
	const char *fn_name;
	size_t	fn_len;
	int	fn_id;
	struct	filename *fn_next;
};

static struct filename *filenames;	/* null-terminated array */
static int next_filename_id;

/* Find the given filename, or return NULL. */
static const struct filename *
search_filename(const char *s, size_t len)
{
	const struct filename *fn;

	for (fn = filenames; fn != NULL; fn = fn->fn_next) {
		if (fn->fn_len == len && memcmp(fn->fn_name, s, len) == 0)
			break;
	}
	return fn;
}

struct filename_replacement {
	const char *orig;
	size_t orig_len;
	const char *repl;
	const struct filename_replacement *next;
};

static struct filename_replacement *filename_replacements;

void
add_directory_replacement(char *arg)
{
	struct filename_replacement *r = xmalloc(sizeof(*r));

	char *sep = strchr(arg, '=');
	if (sep == NULL)
		err(1, "Bad replacement directory spec `%s'", arg);
	*sep = '\0';

	r->orig = arg;
	r->orig_len = (size_t)(sep - arg);
	r->repl = sep + 1;
	r->next = filename_replacements;
	filename_replacements = r;
}

const char *
transform_filename(const char *name, size_t len)
{
	static char buf[MAXPATHLEN];
	const struct filename_replacement *r;

	for (r = filename_replacements; r != NULL; r = r->next)
		if (r->orig_len < len &&
		    memcmp(name, r->orig, r->orig_len) == 0)
			break;
	if (r == NULL)
		return name;
	(void)snprintf(buf, sizeof(buf), "%s%s", r->repl, name + r->orig_len);
	return buf;
}

/*
 * Return a copy of the filename s with unlimited lifetime.
 * If the filename is new, write it to the output file.
 */
const char *
record_filename(const char *s, size_t slen)
{

	const struct filename *existing_fn = search_filename(s, slen);
	if (existing_fn != NULL)
		return existing_fn->fn_name;

	char *name = xmalloc(slen + 1);
	(void)memcpy(name, s, slen);
	name[slen] = '\0';

	struct filename *fn = xmalloc(sizeof(*fn));
	fn->fn_name = name;
	fn->fn_len = slen;
	fn->fn_id = next_filename_id++;
	fn->fn_next = filenames;
	filenames = fn;

	/* Write the ID of this filename to the output file. */
	outclr();
	outint(fn->fn_id);
	outchar('s');
	outstrg(transform_filename(fn->fn_name, fn->fn_len));

	return fn->fn_name;
}

/* Get the ID of a filename. */
int
get_filename_id(const char *s)
{
	const struct filename *fn;

	if (s == NULL || (fn = search_filename(s, strlen(s))) == NULL)
		return -1;
	return fn->fn_id;
}

typedef struct memory_pools {
	struct memory_pool *pools;
	size_t	cap;
} memory_pools;

/* Array of memory pools, indexed by mem_block_level. */
static memory_pools mpools;

/* The pool for the current expression is independent of any block level. */
static memory_pool expr_pool;

static void
mpool_add(memory_pool *pool, struct memory_pool_item item)
{

	if (pool->len >= pool->cap) {
		pool->cap = 2 * pool->len + 16;
		pool->items = xrealloc(pool->items,
		    sizeof(*pool->items) * pool->cap);
	}
	pool->items[pool->len++] = item;
}

static void
mpool_free(memory_pool *pool)
{

	for (; pool->len > 0; pool->len--) {
		struct memory_pool_item *item = pool->items + pool->len - 1;
		void *p = item->p;
#ifdef DEBUG_MEM
		if (strcmp(item->descr, "string") == 0)
			debug_step("%s: freeing string '%s'",
			    __func__, (const char *)p);
		else if (strcmp(item->descr, "sym") == 0)
			debug_step("%s: freeing symbol '%s'",
			    __func__, ((const sym_t *)p)->s_name);
		else if (strcmp(item->descr, "type") == 0)
			debug_step("%s: freeing type '%s'",
			    __func__, type_name(p));
		else if (strcmp(item->descr, "tnode") == 0)
			debug_step("%s: freeing node '%s' with type '%s'",
			    __func__, op_name(((const tnode_t *)p)->tn_op),
			    type_name(((const tnode_t *)p)->tn_type));
		else
			debug_step("%s: freeing '%s' with %zu bytes",
			    __func__, item->descr, item->size);
#endif
		free(p);
	}
}

static void *
#ifdef DEBUG_MEM
mpool_zero_alloc(memory_pool *pool, size_t size, const char *descr)
#else
mpool_zero_alloc(memory_pool *pool, size_t size)
#endif
{

	void *mem = xmalloc(size);
	memset(mem, 0, size);
#if DEBUG_MEM
	mpool_add(pool, (struct memory_pool_item){ mem, size, descr });
#else
	mpool_add(pool, (struct memory_pool_item){ mem });
#endif
	return mem;
}

static memory_pool *
mpool_at(size_t level)
{

	if (level >= mpools.cap) {
		size_t prev_cap = mpools.cap;
		mpools.cap = level + 16;
		mpools.pools = xrealloc(mpools.pools,
		    sizeof(*mpools.pools) * mpools.cap);
		for (size_t i = prev_cap; i < mpools.cap; i++)
			mpools.pools[i] = (memory_pool){ NULL, 0, 0 };
	}
	return mpools.pools + level;
}


/* Allocate memory associated with the level, initialized with zero. */
#ifdef DEBUG_MEM
void *
level_zero_alloc(size_t level, size_t size, const char *descr)
{

	debug_step("%s: %s at level %zu", __func__, descr, level);
	return mpool_zero_alloc(mpool_at(level), size, descr);
}
#else
void *
(level_zero_alloc)(size_t level, size_t size)
{

	return mpool_zero_alloc(mpool_at(level), size);
}
#endif

/* Allocate memory that is freed at the end of the current block. */
#ifdef DEBUG_MEM
void *
block_zero_alloc(size_t size, const char *descr)
{

	return level_zero_alloc(mem_block_level, size, descr);
}
#else
void *
(block_zero_alloc)(size_t size)
{

	return (level_zero_alloc)(mem_block_level, size);
}
#endif

void
level_free_all(size_t level)
{

	debug_step("%s %zu", __func__, level);
	mpool_free(mpool_at(level));
}

/* Allocate memory that is freed at the end of the current expression. */
#if DEBUG_MEM
void *
expr_zero_alloc(size_t s, const char *descr)
{

	return mpool_zero_alloc(&expr_pool, s, descr);
}
#else
void *
(expr_zero_alloc)(size_t size)
{

	return mpool_zero_alloc(&expr_pool, size);
}
#endif

static bool
str_ends_with(const char *haystack, const char *needle)
{
	size_t hlen = strlen(haystack);
	size_t nlen = strlen(needle);

	return nlen <= hlen &&
	       memcmp(haystack + hlen - nlen, needle, nlen) == 0;
}

/*
 * Return a freshly allocated tree node that is freed at the end of the
 * current expression.
 *
 * The node records whether it comes from a system file, which makes strict
 * bool mode less restrictive.
 */
tnode_t *
expr_alloc_tnode(void)
{
	tnode_t *tn = expr_zero_alloc(sizeof(*tn), "tnode");
	/*
	 * files named *.c that are different from the main translation unit
	 * typically contain generated code that cannot be influenced, such
	 * as a flex lexer or a yacc parser.
	 */
	tn->tn_sys = in_system_header ||
		     (curr_pos.p_file != csrc_pos.p_file &&
		      str_ends_with(curr_pos.p_file, ".c"));
	return tn;
}

/* Free all memory which is allocated by the current expression. */
void
expr_free_all(void)
{

	debug_step("%s", __func__);
	mpool_free(&expr_pool);
}

/*
 * Save the memory which is used by the current expression. This memory
 * is not freed by the next expr_free_all() call. The returned value can be
 * used to restore the memory.
 */
memory_pool
expr_save_memory(void)
{

	memory_pool saved_pool = expr_pool;
	expr_pool = (memory_pool){ NULL, 0, 0 };
	return saved_pool;
}

/*
 * Free all memory used for the current expression and restore the memory used
 * by a previous expression and saved by expr_save_memory(). The next call to
 * expr_free_all() frees the restored memory.
 */
void
expr_restore_memory(memory_pool saved_pool)
{

	expr_free_all();
	free(expr_pool.items);
	expr_pool = saved_pool;
}
