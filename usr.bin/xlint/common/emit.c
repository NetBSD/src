/*	$NetBSD: emit.c,v 1.23 2023/08/12 17:13:27 rillig Exp $	*/

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
__RCSID("$NetBSD: emit.c,v 1.23 2023/08/12 17:13:27 rillig Exp $");
#endif

#include <stdio.h>
#include <string.h>

#include "lint.h"

static const char *output_name;
static FILE *output_file;
static bool in_line;

void
outopen(const char *name)
{

	output_name = name;
	if ((output_file = fopen(name, "w")) == NULL)
		err(1, "cannot open '%s'", name);
}

void
outclose(void)
{

	outclr();
	if (fclose(output_file) == EOF)
		err(1, "cannot close '%s'", output_name);
}

void
outclr(void)
{

	if (in_line)
		outchar('\n');
}

void
outchar(char c)
{

	fputc(c, output_file);
	in_line = c != '\n';
}

/*
 * write a string to the output file
 * the string must not contain any characters which should be quoted
 */
void
outstrg(const char *s)
{

	while (*s != '\0')
		outchar(*s++);
}

/* write an integer value to the output file */
void
outint(int i)
{
	char buf[1 + 3 * sizeof(int)];

	snprintf(buf, sizeof(buf), "%d", i);
	outstrg(buf);
}

/* write a name to the output file, preceded by its length */
void
outname(const char *name)
{

	outint((int)strlen(name));
	outstrg(name);
}

/* write the name of the .c source */
void
outsrc(const char *name)
{

	outclr();
	outchar('S');
	outstrg(name);
}
