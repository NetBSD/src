/*	$NetBSD: emit.c,v 1.6 2014/04/17 18:52:03 christos Exp $	*/

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
__RCSID("$NetBSD: emit.c,v 1.6 2014/04/17 18:52:03 christos Exp $");
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "lint.h"

/* name and handle of output file */
static	const	char *loname;
static	FILE	*lout;

/* output buffer data */
ob_t	ob;

static	void	outxbuf(void);


/*
 * initialize output
 */
void
outopen(const char *name)
{

	loname = name;

	/* Open output file */
	if ((lout = fopen(name, "w")) == NULL)
		err(1, "cannot open '%s'", name);

	/* Create output buffer */
	ob.o_len = 1024;
	ob.o_end = (ob.o_buf = ob.o_nxt = xmalloc(ob.o_len)) + ob.o_len;
}

/*
 * flush output buffer and close file
 */
void
outclose(void)
{

	outclr();
	if (fclose(lout) == EOF)
		err(1, "cannot close '%s'", loname);
}

/*
 * resize output buffer
 */
static void
outxbuf(void)
{
	ptrdiff_t coffs;

	coffs = ob.o_nxt - ob.o_buf;
	ob.o_len *= 2;
	ob.o_end = (ob.o_buf = xrealloc(ob.o_buf, ob.o_len)) + ob.o_len;
	ob.o_nxt = ob.o_buf + coffs;
}

/*
 * reset output buffer
 * if it is not empty, it is flushed
 */
void
outclr(void)
{
	size_t	sz;

	if (ob.o_buf != ob.o_nxt) {
		outchar('\n');
		sz = ob.o_nxt - ob.o_buf;
		if (sz > ob.o_len)
			errx(1, "internal error: outclr() 1");
		if (fwrite(ob.o_buf, sz, 1, lout) != 1)
			err(1, "cannot write to %s", loname);
		ob.o_nxt = ob.o_buf;
	}
}

/*
 * write a character to the output buffer
 */
void
outchar(int c)
{

	if (ob.o_nxt == ob.o_end)
		outxbuf();
	*ob.o_nxt++ = (char)c;
}

/*
 * write a character to the output buffer, qouted if necessary
 */
void
outqchar(int c)
{

	if (isprint(c) && c != '\\' && c != '"' && c != '\'') {
		outchar(c);
	} else {
		outchar('\\');
		switch (c) {
		case '\\':
			outchar('\\');
			break;
		case '"':
			outchar('"');
			break;
		case '\'':
			outchar('\'');
			break;
		case '\b':
			outchar('b');
			break;
		case '\t':
			outchar('t');
			break;
		case '\n':
			outchar('n');
			break;
		case '\f':
			outchar('f');
			break;
		case '\r':
			outchar('r');
			break;
		case '\v':
			outchar('v');
			break;
		case '\a':
			outchar('a');
			break;
		default:
			outchar((((u_int)c >> 6) & 07) + '0');
			outchar((((u_int)c >> 3) & 07) + '0');
			outchar((c & 07) + '0');
			break;
		}
	}
}

/*
 * write a strint to the output buffer
 * the string must not contain any characters which
 * should be quoted
 */
void
outstrg(const char *s)
{

	while (*s != '\0') {
		if (ob.o_nxt == ob.o_end)
			outxbuf();
		*ob.o_nxt++ = *s++;
	}
}

/*
 * write an integer value to toe output buffer
 */
void
outint(int i)
{

	if ((size_t)(ob.o_end - ob.o_nxt) < 3 * sizeof (int))
		outxbuf();
	ob.o_nxt += sprintf(ob.o_nxt, "%d", i);
}

/*
 * write the name of a symbol to the output buffer
 * the name is preceded by its length
 */
void
outname1(const char *file, size_t line, const char *name)
{

	if (name == NULL)
		errx(1, "%s, %zu: internal error: outname(NULL)", file, line);
	outint((int)strlen(name));
	outstrg(name);
}

/*
 * write the name of the .c source
 */
void
outsrc(const char *name)
{

	outclr();
	outchar('S');
	outstrg(name);
}
