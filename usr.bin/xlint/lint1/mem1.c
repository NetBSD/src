/*	$NetBSD: mem1.c,v 1.58 2022/02/27 06:55:13 rillig Exp $	*/

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
 *      This product includes software developed by Jochen Pohl for
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
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: mem1.c,v 1.58 2022/02/27 06:55:13 rillig Exp $");
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
	r->orig_len = sep - arg;
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

static int
next_filename_id(void)
{
	static int next_id = 0;

	return next_id++;
}

/*
 * Return a copy of the filename s with unlimited lifetime.
 * If the filename is new, write it to the output file.
 */
const char *
record_filename(const char *s, size_t slen)
{
	const struct filename *existing_fn;
	struct filename *fn;
	char *name;

	if (s == NULL)
		return NULL;

	if ((existing_fn = search_filename(s, slen)) != NULL)
		return existing_fn->fn_name;

	/* Do not use strdup() because s is not NUL-terminated.*/
	name = xmalloc(slen + 1);
	(void)memcpy(name, s, slen);
	name[slen] = '\0';

	fn = xmalloc(sizeof(*fn));
	fn->fn_name = name;
	fn->fn_len = slen;
	fn->fn_id = next_filename_id();
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

/*
 * Memory for declarations and other things which must be available
 * until the end of a block (or the end of the translation unit)
 * is associated with the corresponding mem_block_level, which may be 0.
 * Because this memory is allocated in large blocks associated with
 * a given level it can be freed easily at the end of a block.
 */
#define	ML_INC	((size_t)32)		/* Increment for length of *mblks */

typedef struct memory_block {
	void	*start;			/* beginning of memory block */
	void	*first_free;		/* first free byte */
	size_t	nfree;			/* # of free bytes */
	size_t	size;			/* total size of memory block */
	struct	memory_block *next;
} memory_block;

/*
 * Array of pointers to lists of memory blocks. mem_block_level is used as
 * index into this array.
 */
static	memory_block	**mblks;

/* number of elements in *mblks */
static	size_t	nmblks;

/* length of new allocated memory blocks */
static	size_t	mblklen;


static memory_block *
xnewblk(void)
{
	memory_block	*mb = xmalloc(sizeof(*mb));

	mb->start = xmalloc(mblklen);
	mb->size = mblklen;

	return mb;
}

/* Allocate new memory, initialized with zero. */
static void *
xgetblk(memory_block **mbp, size_t s)
{
	memory_block	*mb;
	void	*p;
	size_t	t = 0;

	/*
	 * If the first block of the list has not enough free space,
	 * or there is no first block, get a new block. The new block
	 * is taken from the free list or, if there is no block on the
	 * free list, is allocated using xnewblk().
	 *
	 * If a new block is allocated it is initialized with zero.
	 * Blocks taken from the free list are zero'd in xfreeblk().
	 */

	s = WORST_ALIGN(s);
	if ((mb = *mbp) == NULL || mb->nfree < s) {
		if (s > mblklen) {
			t = mblklen;
			mblklen = s;
		}
		mb = xnewblk();
#ifndef BLKDEBUG
		(void)memset(mb->start, 0, mb->size);
#endif
		if (t > 0)
			mblklen = t;
		mb->first_free = mb->start;
		mb->nfree = mb->size;
		mb->next = *mbp;
		*mbp = mb;
	}
	p = mb->first_free;
	mb->first_free = (char *)mb->first_free + s;
	mb->nfree -= s;
#ifdef BLKDEBUG
	(void)memset(p, 0, s);
#endif
	return p;
}

/* Free all blocks from list *fmbp. */
static void
xfreeblk(memory_block **fmbp)
{
	memory_block	*mb;

	while ((mb = *fmbp) != NULL) {
		*fmbp = mb->next;
		free(mb);
	}
}

void
initmem(void)
{

	mblklen = mem_block_size();
	mblks = xcalloc(nmblks = ML_INC, sizeof(*mblks));
}


/* Allocate memory associated with level l, initialized with zero. */
void *
getlblk(size_t l, size_t s)
{

	while (l >= nmblks) {
		mblks = xrealloc(mblks, (nmblks + ML_INC) * sizeof(*mblks));
		(void)memset(&mblks[nmblks], 0, ML_INC * sizeof(*mblks));
		nmblks += ML_INC;
	}
	return xgetblk(&mblks[l], s);
}

/*
 * Return allocated memory for the current mem_block_level, initialized with
 * zero.
 */
void *
getblk(size_t s)
{

	return getlblk(mem_block_level, s);
}

/* Free all memory associated with level l. */
void
freelblk(int l)
{

	xfreeblk(&mblks[l]);
}

void
freeblk(void)
{

	freelblk(mem_block_level);
}

static	memory_block	*tmblk;

/*
 * Return zero-initialized memory that is freed at the end of the current
 * expression.
 */
void *
expr_zalloc(size_t s)
{

	return xgetblk(&tmblk, s);
}

static bool
str_endswith(const char *haystack, const char *needle)
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
expr_zalloc_tnode(void)
{
	tnode_t *tn = expr_zalloc(sizeof(*tn));
	/*
	 * files named *.c that are different from the main translation unit
	 * typically contain generated code that cannot be influenced, such
	 * as a flex lexer or a yacc parser.
	 */
	tn->tn_sys = in_system_header ||
		     (curr_pos.p_file != csrc_pos.p_file &&
		      str_endswith(curr_pos.p_file, ".c"));
	return tn;
}

/* Free all memory which is allocated by the current expression. */
void
expr_free_all(void)
{

	xfreeblk(&tmblk);
}

/*
 * Save the memory which is used by the current expression. This memory
 * is not freed by the next expr_free_all() call. The pointer returned can be
 * used to restore the memory.
 */
memory_block *
expr_save_memory(void)
{
	memory_block	*tmem;

	tmem = tmblk;
	tmblk = NULL;
	return tmem;
}

/*
 * Free all memory used for the current expression and restore the memory used
 * by a previous expression and saved by expr_save_memory(). The next call to
 * expr_free_all() frees the restored memory.
 */
void
expr_restore_memory(memory_block *tmem)
{

	expr_free_all();
	if (tmblk != NULL) {
		free(tmblk->start);
		free(tmblk);
	}
	tmblk = tmem;
}
