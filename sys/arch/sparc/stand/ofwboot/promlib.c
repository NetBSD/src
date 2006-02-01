/*	$NetBSD: promlib.c,v 1.1.4.2 2006/02/01 14:51:37 yamt Exp $ */

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * OPENPROM functions.  These are here mainly to hide the OPENPROM interface
 * from the rest of the kernel.
 */

#include <sys/types.h>
#include <machine/promlib.h>

#include "openfirm.h"


void *romp;
struct promops	promops;


static void
openfirmware_fatal(void)
{
	printf("Invalid Openfirmware environment\n");
	exit(0);
}

static int
openfirmware_chosen(void)
{
	static int phandle = -1;

	if (phandle == -1) {
		if ( (phandle = OF_finddevice("/chosen")) == -1) {
			exit(0);
		}
	}

	return (phandle);
}

static const char*
openfirmware_bootpath(void)
{
	static char bootpath[PROM_MAX_PATH];

	if (_prom_getprop(openfirmware_chosen(), "bootpath", bootpath,
				sizeof(bootpath)) < 0) {
		openfirmware_fatal();
	}

	return bootpath;
}

static const char*
openfirmware_bootfile(void)
{
	/* Default image name */
	return "netbsd";
}

static const char*
openfirmware_bootargs(void)
{
	static char bootargs[PROM_MAX_PATH * 2];

	if (_prom_getprop(openfirmware_chosen(), "bootargs", bootargs,
				sizeof(bootargs)) < 0) {
		openfirmware_fatal();
	}

	return bootargs;
}

static int
openfirmware_getchar(void)
{
	unsigned char ch = '\0';
	int l;

	while ((l = OF_read(prom_stdin(), &ch, 1)) != 1)
		if (l != -2 && l != 0)
			return -1;
	return ch;
}

static void
openfirmware_putchar(int c)
{
	char ch = c;

	if (c == '\n')
		putchar('\r');
	OF_write(prom_stdout(), &ch, 1);
}

void
prom_halt(void)
{
	_prom_halt();
}

int
prom_findroot(void)
{
	return OF_peer(0);
}

void
prom_init(void)
{
	int phandle, size;

	OF_initialize();

	memset(promops, 0, sizeof(promops));

	/* Access to boot arguments */
	promops.po_bootpath = openfirmware_bootpath;
	promops.po_bootfile = openfirmware_bootfile;
	promops.po_bootargs = openfirmware_bootargs;

	/* I/O functions */
	promops.po_getchar = openfirmware_getchar;
	promops.po_putchar = openfirmware_putchar;
	promops.po_open  = OF_open;
	promops.po_close = OF_close;
	promops.po_read  = OF_read;
	promops.po_write = OF_write;
	promops.po_seek  = OF_seek;

	promops.po_instance_to_package = OF_instance_to_package;

	/* Program termination control */
	promops.po_halt  = OF_exit;
	promops.po_abort = OF_enter;
	promops.po_ticks = OF_milliseconds;

	/* Device node traversal */
	promops.po_firstchild  = OF_child;
	promops.po_nextsibling = OF_peer;

	/* Device node properties */
	promops.po_getprop = OF_getprop;

	/* Device discovery */
	promops.po_finddevice = OF_finddevice;

	/* Console I/O */
	phandle = openfirmware_chosen();
	size = _prom_getprop(phandle, "stdin", &promops.po_stdin,
			sizeof(promops.po_stdin));
	size += _prom_getprop(phandle, "stdout", &promops.po_stdout,
			sizeof(promops.po_stdout));
	if (size != (sizeof(promops.po_stdin) + sizeof(promops.po_stdout))) {
		prom_halt();
	}
}
