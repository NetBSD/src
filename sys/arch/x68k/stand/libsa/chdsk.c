/*	$NetBSD: chdsk.c,v 1.4 2011/04/11 14:00:02 tsutsui Exp $	*/

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
#include <lib/libkern/libkern.h>

#include "libx68k.h"
#include "iocs.h"
#include "fdvar.h"
#include "consio.h"

int
changedisk_hook(struct open_file *f)
{

	if (strcmp(f->f_dev->dv_name, "fd") == 0) {
		struct fd_softc *sc = f->f_devdata;
		int drive[2];

		drive[0] = 0x90 << 8;
		drive[1] = 0x91 << 8;

		/* unlock current unit */
		IOCS_B_DRVCHK(drive[sc->unit], 3);
		/* eject current */
		IOCS_B_DRVCHK(drive[sc->unit], 1);
		awaitkey_1sec();
		/* prompt both */
		IOCS_B_DRVCHK(drive[0], 4);
		IOCS_B_DRVCHK(drive[1], 4);
		/* poll for medium */
		for (;;) {
			if ((IOCS_B_DRVCHK(drive[0], 0) & 2)) {
				sc->unit = 0;
				break;
			}
			if ((IOCS_B_DRVCHK(drive[1], 0) & 2)) {
				sc->unit = 1;
				break;
			}
			awaitkey_1sec();
		}
		/* prompt off */
		IOCS_B_DRVCHK(drive[0], 5);
		IOCS_B_DRVCHK(drive[1], 5);
		/* lock new unit */
		IOCS_B_DRVCHK(drive[sc->unit], 2);
	}

	return 0;
}
