/*	$NetBSD: chdsk.c,v 1.1 2001/09/28 15:19:33 minoura Exp $	*/

/*
 * Copyright (c) 2001 MINOURA Makoto.
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

#include <lib/libsa/stand.h>

#include "libx68k.h"
#include "iocs.h"
#include "fdvar.h"
#include "consio.h"

int
changedisk_hook(struct open_file *f)
{
	if (strcmp(f->f_dev->dv_name, "fd") == 0) {
		struct fd_softc *sc = f->f_devdata;
		int unit = (0x90 + sc->unit) << 8;

		/* unlock */
		IOCS_B_DRVCHK(unit, 3);
		/* eject */
		IOCS_B_DRVCHK(unit, 1);
		awaitkey_1sec();
		/* prompt */
		IOCS_B_DRVCHK(unit, 4);
		/* poll for medium */
		while ((IOCS_B_DRVCHK(unit, 0) & 2) == 0)
			awaitkey_1sec();
		/* lock */
		IOCS_B_DRVCHK(unit, 2);
	}

	return 0;
}
