/*	$NetBSD: yamon.c,v 1.1.12.2 2002/03/07 14:44:00 simonb Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* XXX move to arch/mips/yamon/yamon.c or similar? */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/cons.h>

#include <machine/yamon.h>

static int  yamongetc(dev_t);
static void yamonputc(dev_t, int);

/*
 * Default consdev, for errors or warnings before
 * consinit runs: use the PROM.
 */
struct consdev yamon_promcd = {
	NULL,		/* probe */
	NULL,		/* init */
	yamongetc,	/* getc */
	yamonputc,	/* putc */
	nullcnpollc,	/* pollc */
	NULL,		/* bell */
	makedev(0, 0),
	CN_DEAD,
};
/*
 * Get character from PROM console.
 */
static int
yamongetc(dev)
	dev_t dev;
{
	char chr;

	while (!YAMON_GETCHAR(&chr))
		/* nothing */;
	return chr;
}

/*
 * Print a character on PROM console.
 */
static void
yamonputc(dev_t dev, int c)
{
	char chr;

	chr = c;
	YAMON_PRINT_COUNT(&chr, 1);
}

char *
yamon_getenv(char *name)
{
	yamon_env_var *yev = yamon_envp;

	if (yev == NULL)
		return (NULL);
	while (yev->name != NULL) {
		if (strcmp(yev->name, name) == 0)
			return (yev->val);
		yev++;
	}

	return (NULL);
}

void
yamon_print(char *str)
{

	YAMON_PRINT(str);
}

void
yamon_exit(uint32_t rc)
{

	YAMON_EXIT(rc);
}
