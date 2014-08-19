/*	$NetBSD: cons.c,v 1.1.110.1 2014/08/20 00:03:09 tls Exp $	*/

/*-
 * Copyright (c) 2005 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <sys/bootblock.h>

#include "boot.h"
#include "cons.h"

#ifndef	CONSPEED
#define	CONSPEED	9600
#endif

#define	POLL_FREQ	10

extern struct landisk_boot_params boot_params;

static int consdev = CONSDEV_BIOSCONS;

/*ARGSUSED*/
int
cninit(int dev)
{

	switch (dev) {
	default:
	case CONSDEV_BIOSCONS:
		break;

	case CONSDEV_SCIF:
		switch (boot_params.bp_conspeed) {
		default:
			scif_init(CONSPEED);
			break;

		case 9600:
#if 0
		case 19200:
		case 38400:
		case 57600:
		case 115200:
#endif
			scif_init(boot_params.bp_conspeed);
			break;
		}
		break;
	}
	consdev = dev;

	return (0);
}

int
getchar(void)
{

	switch (consdev) {
	default:
	case CONSDEV_BIOSCONS:
		return bioscons_getc();

	case CONSDEV_SCIF:
		return scif_getc();
	}
}

void
putchar(int c)
{

	switch (consdev) {
	default:
	case CONSDEV_BIOSCONS:
		bioscons_putc(c);
		break;

	case CONSDEV_SCIF:
		if (c == '\n')
			scif_putc('\r');
		scif_putc(c);
		break;
	}
}

/*ARGSUSED*/
int
iskey(int intr)
{

	switch (consdev) {
	default:
	case CONSDEV_BIOSCONS:
		return scif_status2();

	case CONSDEV_SCIF:
		return scif_status();
	}
}

char
awaitkey(int timeout, int tell)
{
	int i;
	char c = 0;

	i = timeout * POLL_FREQ;

	for (;;) {
		if (tell && (i % POLL_FREQ) == 0) {
			char numbuf[20];
			int len, j;

			len = snprintf(numbuf, sizeof(numbuf),
			    "%d ", i / POLL_FREQ);
			for (j = 0; j < len; j++)
				numbuf[len + j] = '\b';
			numbuf[len + j] = '\0';
			printf(numbuf);
		}
		if (iskey(1)) {
			/* flush input buffer */
			while (iskey(0))
				c = getchar();
			if (c == 0)
				c = -1;
			goto out;
		}
		if (i--) {
			delay(1000 / POLL_FREQ);
		} else {
			break;
		}
	}

out:
	if (tell)
		printf("0 \n");

	return (c);
}
