/*	$NetBSD: bootinfo_biosgeom.c,v 1.2 1999/01/27 20:54:57 thorpej Exp $	*/

/*
 * Copyright (c) 1997
 *	Matthias Drochner.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
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
 *
 */

#include <sys/types.h>
#include <machine/disklabel.h>

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>

#include "libi386.h"
#include "biosdisk_ll.h"
#include "bootinfo.h"

void bi_getbiosgeom()
{
	unsigned char nhd;
	struct btinfo_biosgeom *bibg;
	int i;

	pvbcopy(0x400 + 0x75, &nhd, 1); /* from BIOS data area */
	if(nhd == 0 || nhd > 4 /* ??? */ )
		return;

	bibg = alloc(sizeof(struct btinfo_biosgeom)
		     + (nhd - 1) * sizeof(struct bi_biosgeom_entry));
	if(!bibg) return;

	bibg->num = nhd;

	for(i = 0; i < nhd; i++) {
		struct biosdisk_ll d;
		char buf[BIOSDISK_SECSIZE];

		d.dev = 0x80 + i;
		set_geometry(&d);
		bibg->disk[i].spc = d.spc;
		bibg->disk[i].spt = d.spt;

		bzero(bibg->disk[i].dosparts,
		      sizeof(bibg->disk[i].dosparts));
		if(readsects(&d, 0, 1, buf, 0))
			continue;
		bcopy(&buf[MBR_PARTOFF], bibg->disk[i].dosparts,
		      sizeof(bibg->disk[i].dosparts));
	}

	BI_ADD(bibg, BTINFO_BIOSGEOM, sizeof(struct btinfo_biosgeom)
	       + (nhd - 1) * sizeof(struct bi_biosgeom_entry));
}
