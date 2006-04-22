/* $NetBSD: au_chipdep.c,v 1.1.10.2 2006/04/22 11:37:41 simonb Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: au_chipdep.c,v 1.1.10.2 2006/04/22 11:37:41 simonb Exp $");

#include <sys/param.h>
#include <machine/bus.h>
#include <machine/locore.h>
#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>

struct au_chipdep *
au_chipdep(void)
{

	static struct au_chipdep *au_chip = NULL;

	if (au_chip != NULL)
		return (au_chip);

	if ((au_chip == NULL) &&
#ifdef	ALCHEMY_AU1000
	    (!au1000_match(&au_chip)) &&
#endif
#ifdef	ALCHEMY_AU1100
	    (!au1100_match(&au_chip)) &&
#endif
#ifdef	ALCHEMY_AU1500
	    (!au1500_match(&au_chip)) &&
#endif
#ifdef	ALCHEMY_AU1550
	    (!au1550_match(&au_chip)) &&
#endif
	    (au_chip == NULL)) {
		panic("Alchemy SOC %x either not configured or not supported!",
		    cpu_id);
	}
	return (au_chip);
}
