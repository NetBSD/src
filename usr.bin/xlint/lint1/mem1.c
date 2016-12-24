/*	$NetBSD: mem1.c,v 1.18 2016/12/24 17:43:45 christos Exp $	*/

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
__RCSID("$NetBSD: mem1.c,v 1.18 2016/12/24 17:43:45 christos Exp $");
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lint1.h"

/*
 * Filenames allocated by fnalloc() and fnnalloc() are shared.
 */
typedef struct fn {
	char	*fn_name;
	size_t	fn_len;
	int	fn_id;
	struct	fn *fn_nxt;
} fn_t;

static	fn_t	*fnames;

static	fn_t	*srchfn(const char *, size_t);

/*
 * Look for a Filename of length l.
 */
static fn_t *
srchfn(const char *s, size_t len)
{
	fn_t	*fn;

	for (fn = fnames; fn != NULL; fn = fn->fn_nxt) {
		if (fn->fn_len == len && memcmp(fn->fn_name, s, len) == 0)
			break;
	}
	return (fn);
}

/*
 * Return a shared string for filename s.
 */
const char *
fnalloc(const char *s)
{

	return (s != NULL ? fnnalloc(s, strlen(s)) : NULL);
}

struct repl {
	char *orig;
	char *repl;
	size_t len;
	struct repl *next;
};

struct repl *replist;

void
fnaddreplsrcdir(char *arg)
{
	struct repl *r = xmalloc(sizeof(*r));
	
	r->orig = arg;
	if ((r->repl = strchr(arg, '=')) == NULL) 
		err(1, "Bad replacement directory spec `%s'", arg);
	r->len = r->repl - r->orig;
	*(r->repl)++ = '\0';
	if (replist == NULL) {
		r->next = NULL;
	} else
		r->next = replist;
	replist = r;
}

const char *
fnxform(const char *name, size_t len)
{
	static char buf[MAXPATHLEN];
	struct repl *r;

	for (r = replist; r; r = r->next)
		if (r->len < len && memcmp(name, r->orig, r->len) == 0)
			break;
	if (r == NULL)
		return name;
	snprintf(buf, sizeof(buf), "%s%s", r->repl, name + r->len);
	return buf;
}

const char *
fnnalloc(const char *s, size_t len)
{
	fn_t	*fn;

	static	int	nxt_id = 0;

	if (s == NULL)
		return (NULL);

	if ((fn = srchfn(s, len)) == NULL) {
		fn = xmalloc(sizeof (fn_t));
		/* Do not used strdup() because string is not NUL-terminated.*/
		fn->fn_name = xmalloc(len + 1);
		(void)memcpy(fn->fn_name, s, len);
		fn->fn_name[len] = '\0';
		fn->fn_len = len;
		fn->fn_id = nxt_id++;
		fn->fn_nxt = fnames;
		fnames = fn;
		/* Write id of this filename to the output file. */
		outclr();
		outint(fn->fn_id);
		outchar('s');
		outstrg(fnxform(fn->fn_name, fn->fn_len));
	}
	return (fn->fn_name);
}

/*
 * Get id of a filename.
 */
int
getfnid(const char *s)
{
	fn_t	*fn;

	if (s == NULL || (fn = srchfn(s, strlen(s))) == NULL)
		return (-1);
	return (fn->fn_id);
}

/*
 * Memory for declarations and other things which must be available
 * until the end of a block (or the end of the translation unit)
 * are associated with the level (mblklev) of the block (or with 0).
 * Because this memory is allocated in large blocks associated with
 * a given level it can be freed easily at the end of a block.
 */
#define	ML_INC	((size_t)32)		/* Increment for length of *mblks */

typedef struct mbl {
	void	*blk;			/* beginning of memory block */
	void	*ffree;			/* first free byte */
	size_t	nfree;			/* # of free bytes */
	size_t	size;			/* total size of memory block */
	struct	mbl *nxt;		/* next block */
} mbl_t;

/*
 * Array of pointers to lists of memory blocks. mblklev is used as
 * index into this array.
 */
static	mbl_t	**mblks;

/* number of elements in *mblks */
static	size_t	nmblks;

/* free list for memory blocks */
static	mbl_t	*frmblks;

/* length of new allocated memory blocks */
static	size_t	mblklen;

static	void	*xgetblk(mbl_t **, size_t);
static	void	xfreeblk(mbl_t **);
static	mbl_t	*xnewblk(void);

static mbl_t *
xnewblk(void)
{
	mbl_t	*mb = xmalloc(sizeof (mbl_t));

	/* use mmap instead of malloc to avoid malloc's size overhead */
	mb->blk = xmapalloc(mblklen);
	mb->size = mblklen;

	return (mb);
}

/*
 * Allocate new memory. If the first block of the list has not enough
 * free space, or there is no first block, get a new block. The new
 * block is taken from the free list or, if there is no block on the
 * free list, is allocated using xnewblk(). If a new block is allocated
 * it is initialized with zero. Blocks taken from the free list are
 * zero'd in xfreeblk().
 */
static void *
xgetblk(mbl_t **mbp, size_t s)
{
	mbl_t	*mb;
	void	*p;
	size_t	t = 0;

	s = WORST_ALIGN(s);
	if ((mb = *mbp) == NULL || mb->nfree < s) {
		if ((mb = frmblks) == NULL || mb->size < s) {
			if (s > mblklen) {
				t = mblklen;
				mblklen = s;
			}
			mb = xnewblk();
#ifndef BLKDEBUG
			(void)memset(mb->blk, 0, mb->size);
#endif
			if (t)
				mblklen = t;
		} else {
			frmblks = mb->nxt;
		}
		mb->ffree = mb->blk;
		mb->nfree = mb->size;
		mb->nxt = *mbp;
		*mbp = mb;
	}
	p = mb->ffree;
	mb->ffree = (char *)mb->ffree + s;
	mb->nfree -= s;
#ifdef BLKDEBUG
	(void)memset(p, 0, s);
#endif
	return (p);
}

/*
 * Move all blocks from list *fmbp to free list. For each block, set all
 * used memory to zero.
 */
static void
xfreeblk(mbl_t **fmbp)
{
	mbl_t	*mb;

	while ((mb = *fmbp) != NULL) {
		*fmbp = mb->nxt;
		mb->nxt = frmblks;
		frmblks = mb;
		(void)memset(mb->blk, ZERO, mb->size - mb->nfree);
	}
}

void
initmem(void)
{
	int	pgsz;

	pgsz = getpagesize();
	mblklen = ((MBLKSIZ + pgsz - 1) / pgsz) * pgsz;

	mblks = xcalloc(nmblks = ML_INC, sizeof (mbl_t *));
}


/*
 * Allocate memory associated with level l.
 */
void *
getlblk(size_t l, size_t s)
{

	while (l >= nmblks) {
		mblks = xrealloc(mblks, (nmblks + ML_INC) * sizeof (mbl_t *));
		(void)memset(&mblks[nmblks], 0, ML_INC * sizeof (mbl_t *));
		nmblks += ML_INC;
	}
	return (xgetblk(&mblks[l], s));
}

void *
getblk(size_t s)
{

	return (getlblk(mblklev, s));
}

/*
 * Free all memory associated with level l.
 */
void
freelblk(int l)
{

	xfreeblk(&mblks[l]);
}

void
freeblk(void)
{

	freelblk(mblklev);
}

/*
 * tgetblk() returns memory which is associated with the current
 * expression.
 */
static	mbl_t	*tmblk;

void *
tgetblk(size_t s)
{

	return (xgetblk(&tmblk, s));
}

/*
 * Get memory for a new tree node.
 */
tnode_t *
getnode(void)
{

	return (tgetblk(sizeof (tnode_t)));
}

/*
 * Free all memory which is allocated by the current expression.
 */
void
tfreeblk(void)
{

	xfreeblk(&tmblk);
}

/*
 * Save the memory which is used by the current expression. This memory
 * is not freed by the next tfreeblk() call. The pointer returned can be
 * used to restore the memory.
 */
mbl_t *
tsave(void)
{
	mbl_t	*tmem;

	tmem = tmblk;
	tmblk = NULL;
	return (tmem);
}

/*
 * Free all memory used for the current expression and the memory used
 * be a previous expression and saved by tsave(). The next call to
 * tfreeblk() frees the restored memory.
 */
void
trestor(mbl_t *tmem)
{

	tfreeblk();
	if (tmblk != NULL) {
		free(tmblk->blk);
		free(tmblk);
	}
	tmblk = tmem;
}
