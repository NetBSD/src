/* $NetBSD: autoconf.c,v 1.1 2002/03/24 15:46:42 bjh21 Exp $ */
/*-
 * Copyright (c) 1998, 1999 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * autoconf.c - top level device-finding code
 */

#include <sys/param.h>

__RCSID("$NetBSD: autoconf.c,v 1.1 2002/03/24 15:46:42 bjh21 Exp $");

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/intr.h>
#include <machine/irq.h>
#include <machine/machdep.h>

struct device *booted_device;

void
cpu_configure()
{

	irq_init();
	config_rootfound("cpu", NULL);
	config_rootfound("iobus", NULL);
	config_rootfound("arcvideo", NULL);
	spl0();
	fiq_on();
}

void
cpu_rootconf()
{

	/* XXX This could actually do something */
	setroot(NULL, 0);
}

void
cpu_dumpconf()
{

	printf("cpu_dumpconf: Doing something here would be useful\n");
}
