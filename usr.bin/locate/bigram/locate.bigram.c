/*	$NetBSD: locate.bigram.c,v 1.12 2009/11/17 18:31:12 drochner Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James A. Woods.
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
__COPYRIGHT("@(#) Copyright (c) 1989, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)locate.bigram.c	8.2 (Berkeley) 4/28/95";
#endif
__RCSID("$NetBSD: locate.bigram.c,v 1.12 2009/11/17 18:31:12 drochner Exp $");
#endif /* not lint */

/*
 *  bigram < text > bigrams
 * 
 * List bigrams for 'updatedb' script.
 * Use 'code' to encode a file using this output.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>			/* for MAXPATHLEN */

int	main(int, char **);
static int compare_bigrams(const void *, const void *);
static void add_bigram(u_char, u_char);

static char buf1[MAXPATHLEN] = " ";	
static char buf2[MAXPATHLEN];

struct bigram {
	int count;
	u_char b1, b2;	/* needed for final sorting */
};

struct bigram bigrams[256 * 256];

static void
add_bigram(u_char i1, u_char i2)
{
	if (i1 != '\n' && i2 != '\n')
		bigrams[(i1<<8)+i2].count++;
}

static int
compare_bigrams(const void *item1, const void *item2)
{
	const struct bigram *it1=item1, *it2=item2;

	if (it1->count != it2->count)
		return it2->count - it1->count;
	else if (it1->b1 != it2->b1)
		return it2->b1 - it1->b1;
	else
		return it2->b2 - it2->b2;
}

int
main(int argc, char *argv[])
{
  	char *cp;
	char *oldpath = buf1, *path = buf2;
	struct bigram *bg;
	int i;

	/* initialize bigram array */
	memset(bigrams, 0, sizeof(bigrams));
	for(i=0; i < 65536; i++) {
		bigrams[i].b1 = i / 256;
		bigrams[i].b2 = i % 256;
	}

     	while ( fgets ( path, sizeof(buf2), stdin ) != NULL ) {

		/* skip longest common prefix */
		for ( cp = path; *cp == *oldpath; cp++, oldpath++ )
			if ( *oldpath == '\0' )
				break;

		/*
		 * output post-residue bigrams only
		 */
		for(; cp[0] != '\0' && cp[1] != '\0'; cp += 2)
			add_bigram((u_char)cp[0], (u_char)cp[1]);

		if (path == buf1)		/* swap pointers */
			path = buf2, oldpath = buf1;
		else
			path = buf1, oldpath = buf2;
   	}

	/* sort the bigrams by how many times it appeared and their value */
	heapsort((void *)bigrams, 256 * 256, sizeof(struct bigram),
			compare_bigrams);

	/* write 128 most frequent bigrams out */
	bg = bigrams;
	for (i = 0; i < 128 && bg->count > 0; i++, bg++) {
		if (bg->b1 != '\0')
			fputc(bg->b1, stdout);
		if (bg->b2 != '\0')
			fputc(bg->b2, stdout);
	}
		
	return (0);
}
