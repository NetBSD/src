/*	$NetBSD: netbsd_opts.c,v 1.3 2000/07/29 20:06:27 jdolecek Exp $	*/

/*
 * Copyright (c) 1996
 * 	Matthias Drochner.  All rights reserved.
 * Copyright (c) 1996
 * 	Perry E. Metzger.  All rights reserved.
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
 *    must display the following acknowledgements:
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 *	This product includes software developed for the NetBSD Project
 *	by Perry E. Metzger.
 * 4. The names of the authors may not be used to endorse or promote products
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


#include <sys/reboot.h>

#include <lib/libsa/stand.h>

#include "libi386.h"

static struct optent {
	char name;
	int flag;
} opttab[] = {
	{'a', RB_ASKNAME},
	{'s', RB_SINGLE},
	{'d', RB_KDB},
};

#define OPTTABSIZE (sizeof(opttab)/sizeof(struct optent))

int
netbsd_opt(c)
	char c;
{
	int i;

	for (i = 0; i < OPTTABSIZE; i++) {
		if (c == opttab[i].name)
			return (opttab[i].flag);
	}
	return (-1);
}
