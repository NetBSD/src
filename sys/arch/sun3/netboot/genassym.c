/*	$NetBSD: genassym.c,v 1.3 1995/03/28 18:38:45 jtc Exp $	*/

/*
 * Copyright (c) 1993 Adam Glass 
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 *	@(#)genassym.c	7.8 (Berkeley) 5/7/91
 */

#define KERNEL
#define _KERNEL

#include "sys/cdefs.h"
#include "sys/errno.h"

#include "machine/memmap.h"
#include "machine/cpu.h"
#include "machine/trap.h"
#include "machine/psl.h"
#include "machine/pte.h"
#include "machine/control.h"
#include "machine/mon.h"
#include "machine/param.h"

#define GENASSYM
#include "config.h"

int main(argc, argv)
     int argc;
     char *argv[];
{
				/* 68k isms */
    printf("#define\tPSL_LOWIPL %d\n", PSL_LOWIPL);
    printf("#define\tPSL_HIGHIPL %d\n", PSL_HIGHIPL);
    printf("#define\tPSL_IPL7 %d\n", PSL_IPL7);
    printf("#define\tPSL_USER %d\n", PSL_USER);
    printf("#define\tSPL1 %d\n", PSL_S | PSL_IPL1);
    printf("#define\tFC_CONTROL %d\n",  FC_CONTROL);

				/* sun3 control space isms */
    printf("#define\tCONTEXT_0 %d\n",   CONTEXT_0);
    printf("#define\tCONTEXT_REG %d\n", CONTEXT_REG);
    printf("#define\tCONTEXT_NUM %d\n", CONTEXT_NUM);
    printf("#define\tSEGMAP_BASE %d\n", SEGMAP_BASE);
    printf("#define\tNBPG %d\n",        NBPG);
    printf("#define\tNBSG %d\n",        NBSG);

				/* sun3 memory map */
    printf("#define\tMAINMEM_MONMAP %d\n",    MAINMEM_MONMAP);
    printf("#define\tMONSHORTSEG %d\n",       MONSHORTSEG);

    printf("#define\tEFAULT %d\n",        EFAULT);
    printf("#define\tENAMETOOLONG %d\n",  ENAMETOOLONG);

    printf("#define\tFIXED_LOAD_ADDR %d\n",  FIXED_LOAD_ADDR);

    exit(0);
}
