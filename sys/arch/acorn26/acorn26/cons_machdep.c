/* $NetBSD: cons_machdep.c,v 1.2 2002/03/24 23:37:42 bjh21 Exp $ */
/*-
 * Copyright (c) 1998 Ben Harris
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
/*
 * cons_machdep.c -- machine dependent console routines
 */

#include <sys/param.h>

__RCSID("$NetBSD: cons_machdep.c,v 1.2 2002/03/24 23:37:42 bjh21 Exp $");

#include <sys/syslog.h>
#include <sys/systm.h>

#include <dev/cons.h>

#include "arcvideo.h"
#include "opt_ddb.h"

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#include <ddb/db_interface.h>

#include <machine/boot.h>
#include <machine/memcreg.h>
#endif

extern void arccons_init __P((void));

void
consinit()
{

#if NARCVIDEO > 0
	arccons_init();
#endif
/*       	cninit();*/

#ifdef DDB
	db_machine_init();
	ddb_init(bootconfig.esym - bootconfig.ssym,
		 MEMC_PHYS_BASE + bootconfig.ssym,
		 MEMC_PHYS_BASE + bootconfig.esym);
#endif /* DDB */
}
