/*	$NetBSD: memalloc.c,v 1.37 2022/05/31 08:43:13 andvar Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)memalloc.c	8.3 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: memalloc.c,v 1.37 2022/05/31 08:43:13 andvar Exp $");
#endif
#endif /* not lint */

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "shell.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "machdep.h"
#include "mystring.h"

/*
 * Like malloc, but returns an error when out of space.
 */

pointer
ckmalloc(size_t nbytes)
{
	pointer p;

	p = malloc(nbytes);
	if (p == NULL)
		error("Out of space");
	return p;
}


/*
 * Same for realloc.
 */

pointer
ckrealloc(pointer p, int nbytes)
{
	p = realloc(p, nbytes);
	if (p == NULL)
		error("Out of space");
	return p;
}


/*
 * Make a copy of a string in safe storage.
 */

char *
savestr(const char *s)
{
	char *p;

	p = ckmalloc(strlen(s) + 1);
	scopy(s, p);
	return p;
}


/*
 * Parse trees for commands are allocated in lifo order, so we use a stack
 * to make this more efficient, and also to avoid all sorts of exception
 * handling code to handle interrupts in the middle of a parse.
 *
 * The size 504 was chosen because the Ultrix malloc handles that size
 * well.
 */

#define MINSIZE 504		/* minimum size of a block */

struct stack_block {
	struct stack_block *prev;
	char space[MINSIZE];
};

struct stack_block stackbase;
struct stack_block *stackp = &stackbase;
struct stackmark *markp;
char *stacknxt = stackbase.space;
int stacknleft = MINSIZE;
int sstrnleft;
int herefd = -1;

pointer
stalloc(int nbytes)
{
	char *p;

	nbytes = SHELL_ALIGN(nbytes);
	if (nbytes > stacknleft) {
		int blocksize;
		struct stack_block *sp;

		blocksize = nbytes;
		if (blocksize < MINSIZE)
			blocksize = MINSIZE;
		INTOFF;
		sp = ckmalloc(sizeof(struct stack_block) - MINSIZE + blocksize);
		sp->prev = stackp;
		stacknxt = sp->space;
		stacknleft = blocksize;
		stackp = sp;
		INTON;
	}
	INTOFF;
	p = stacknxt;
	stacknxt += nbytes;
	stacknleft -= nbytes;
	INTON;
	return p;
}


void
stunalloc(pointer p)
{
	if (p == NULL) {		/*DEBUG */
		write(2, "stunalloc\n", 10);
		abort();
	}
	stacknleft += stacknxt - (char *)p;
	stacknxt = p;
}


/* save the current status of the sh stack */
void
setstackmark(struct stackmark *mark)
{
	mark->stackp = stackp;
	mark->stacknxt = stacknxt;
	mark->stacknleft = stacknleft;
	mark->sstrnleft = sstrnleft;
	mark->marknext = markp;
	markp = mark;
}

/* reset the stack mark, and remove it from the list of marks */
void
popstackmark(struct stackmark *mark)
{
	INTOFF;
	markp = mark->marknext;		/* delete mark from the list */
	rststackmark(mark);		/* and reset stack */
	INTON;
}

/* reset the shell stack to its state recorded in the stack mark */
void
rststackmark(struct stackmark *mark)
{
	struct stack_block *sp;

	INTOFF;
	while (stackp != mark->stackp) {
		/* delete any recently allocated mem blocks */
		sp = stackp;
		stackp = sp->prev;
		ckfree(sp);
	}
	stacknxt = mark->stacknxt;
	stacknleft = mark->stacknleft;
	sstrnleft = mark->sstrnleft;
	INTON;
}


/*
 * When the parser reads in a string, it wants to stick the string on the
 * stack and only adjust the stack pointer when it knows how big the
 * string is.  Stackblock (defined in stack.h) returns a pointer to a block
 * of space on top of the stack and stackblocklen returns the length of
 * this block.  Growstackblock will grow this space by at least one byte,
 * possibly moving it (like realloc).  Grabstackblock actually allocates the
 * part of the block that has been used.
 */

void
growstackblock(void)
{
	int newlen = SHELL_ALIGN(stacknleft * 2 + 100);

	INTOFF;
	if (stacknxt == stackp->space && stackp != &stackbase) {
		struct stack_block *oldstackp;
		struct stackmark *xmark;
		struct stack_block *sp;

		oldstackp = stackp;
		sp = stackp;
		stackp = sp->prev;
		sp = ckrealloc((pointer)sp,
		    sizeof(struct stack_block) - MINSIZE + newlen);
		sp->prev = stackp;
		stackp = sp;
		stacknxt = sp->space;
		sstrnleft += newlen - stacknleft;
		stacknleft = newlen;

		/*
		 * Stack marks pointing to the start of the old block
		 * must be relocated to point to the new block 
		 */
		xmark = markp;
		while (xmark != NULL && xmark->stackp == oldstackp) {
			xmark->stackp = stackp;
			xmark->stacknxt = stacknxt;
			xmark->sstrnleft += stacknleft - xmark->stacknleft;
			xmark->stacknleft = stacknleft;
			xmark = xmark->marknext;
		}
	} else {
		char *oldspace = stacknxt;
		int oldlen = stacknleft;
		char *p = stalloc(newlen);

		(void)memcpy(p, oldspace, oldlen);
		stacknxt = p;			/* free the space */
		stacknleft += newlen;		/* we just allocated */
	}
	INTON;
}

void
grabstackblock(int len)
{
	len = SHELL_ALIGN(len);
	INTOFF;
	stacknxt += len;
	stacknleft -= len;
	INTON;
}

/*
 * The following routines are somewhat easier to use than the above.
 * The user declares a variable of type STACKSTR, which may be declared
 * to be a register.  The macro STARTSTACKSTR initializes things.  Then
 * the user uses the macro STPUTC to add characters to the string.  In
 * effect, STPUTC(c, p) is the same as *p++ = c except that the stack is
 * grown as necessary.  When the user is done, she can just leave the
 * string there and refer to it using stackblock().  Or she can allocate
 * the space for it using grabstackstr().  If it is necessary to allow
 * someone else to use the stack temporarily and then continue to grow
 * the string, the user should use grabstack to allocate the space, and
 * then call ungrabstr(p) to return to the previous mode of operation.
 *
 * USTPUTC is like STPUTC except that it doesn't check for overflow.
 * CHECKSTACKSPACE can be called before USTPUTC to ensure that there
 * is space for at least one character.
 */

char *
growstackstr(void)
{
	int len = stackblocksize();
	if (herefd >= 0 && len >= 1024) {
		xwrite(herefd, stackblock(), len);
		sstrnleft = len - 1;
		return stackblock();
	}
	growstackblock();
	sstrnleft = stackblocksize() - len - 1;
	return stackblock() + len;
}

/*
 * Called from CHECKSTRSPACE.
 */

char *
makestrspace(void)
{
	int len = stackblocksize() - sstrnleft;
	growstackblock();
	sstrnleft = stackblocksize() - len;
	return stackblock() + len;
}

/*
 * Note that this only works to release stack space for reuse
 * if nothing else has allocated space on the stack since the grabstackstr()
 *
 * "s" is the start of the area to be released, and "p" represents the end
 * of the string we have stored beyond there and are now releasing.
 * (ie: "p" should be the same as in the call to grabstackstr()).
 *
 * stunalloc(s) and ungrabstackstr(s, p) are almost interchangeable after
 * a grabstackstr(), however the latter also returns string space so we
 * can just continue with STPUTC() etc without needing a new STARTSTACKSTR(s)
 */
void
ungrabstackstr(char *s, char *p)
{
#ifdef DEBUG
	if (s < stacknxt || stacknxt + stacknleft < s)
		abort();
#endif
	stacknleft += stacknxt - s;
	stacknxt = s;
	sstrnleft = stacknleft - (p - s);
}

/*
 * Save the concat of a sequence of strings in stack space
 *
 * The first arg (if not NULL) is a pointer to where the final string
 * length will be returned.
 *
 * Remaining args are pointers to strings - sufficient space to hold
 * the concat of the strings is allocated on the stack, the strings
 * are copied into that space, and a pointer to its start is returned.
 * The arg list is terminated with STSTRC_END.
 *
 * Use stunalloc(string) (in proper sequence) to release the string
 */
char *
ststrcat(size_t *lp, ...)
{
	va_list ap;
	const char *arg;
	size_t len, tlen = 0, alen[8];
	char *str, *nxt;
	unsigned int n;

	n = 0;
	va_start(ap, lp);
	arg = va_arg(ap, const char *);
	while (arg != STSTRC_END) {
		len = strlen(arg);
		if (n < sizeof(alen)/sizeof(alen[0]))
			alen[n++] = len;
		tlen += len;
		arg = va_arg(ap, const char *);
	}
	va_end(ap);

	if (lp != NULL)
		*lp = tlen;

	if (tlen >= INT_MAX)
		error("ststrcat() over length botch");
	str = (char *)stalloc((int)tlen + 1);	/* 1 for \0 */
	str[tlen] = '\0';	/* in case of no args  */

	n = 0;
	nxt = str;
	va_start(ap, lp);
	arg = va_arg(ap, const char *);
	while (arg != STSTRC_END) {
		if (n < sizeof(alen)/sizeof(alen[0]))
			len = alen[n++];
		else
			len = strlen(arg);

		scopy(arg, nxt);
		nxt += len;

		arg = va_arg(ap, const char *);
	}
	va_end(ap);

	return str;
}

