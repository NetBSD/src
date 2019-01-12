/*	$NetBSD: iris_cons.c,v 1.1 2019/01/12 16:44:47 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Naruaki Etomi
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

/*
 * Silicon Graphics "IRIS" series MIPS processors machine bootloader.
 * Front end of Zilog Z8530 Dual UART driver.
 */

#include <lib/libsa/stand.h>

#include <machine/cpu.h>

#include "iris_machdep.h"
#include "iris_cons.h"
#include "iris_zs.h"

void zscnprobe(struct consdev *);
void zscninit(struct consdev *);
void zscnputchar(void *, int);
int zscngetchar(void *);
int zscnscan(void *);

#define ZSCHAN 0x0
#define ZSSPEED 9600

struct consdev constab[] = {
	{ "zs", ZSCHAN, ZSSPEED,
		zscnprobe, zscninit, zscngetchar, zscnputchar, zscnscan },
	{ 0 }
};

struct consdev *cn_tab;

char *
cninit(int *addr, int *speed)
{
	struct consdev *cp;

	cn_tab = NULL;
	for (cp = constab; cp->cn_probe; cp++) {
		(*cp->cn_probe)(cp);
		if (cp->cn_pri > CN_DEAD &&
		    (cn_tab == NULL || cp->cn_pri > cn_tab->cn_pri))
			cn_tab = cp;
	}
	if (cn_tab) {
		(*cn_tab->cn_init)(cn_tab);
		*addr = cn_tab->address;
		*speed = cn_tab->speed;
		return cn_tab->cn_name;
	}

	return NULL;
}

int
cngetc(void)
{

	if (cn_tab)
		return (*cn_tab->cn_getc)(cn_tab->cn_dev);
	return 0;
}

void
cnputc(int c)
{

	if (cn_tab)
		(*cn_tab->cn_putc)(cn_tab->cn_dev, c);
}

int
cnscan(void)
{

	if (cn_tab)
		return (*cn_tab->cn_scan)(cn_tab->cn_dev);
	return -1;
}

void
zscnprobe(struct consdev *cp)
{

	cp->cn_pri = CN_REMOTE;
}

void
zscninit(struct consdev *cp)
{

	cp->cn_dev = zs_init(cp->address, cp->speed);
}

int
zscngetchar(void *dev)
{

	return zscngetc(dev);
}

void
zscnputchar(void *dev, int c)
{

	if (c == '\n')
		zscnputc(dev, '\r');
	zscnputc(dev, c);
}

int
zscnscan(void *dev)
{

	return zscnscanc(dev);
}
