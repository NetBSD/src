/*	$NetBSD: init_board.c,v 1.1.8.2 2011/06/06 09:05:21 jruoho Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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

#include <sys/types.h>
#include <mips/cpuregs.h>
#include <stand/common/common.h>
#include <stand/common/prom_iface.h>

int board_config = 0;
int mem_config = 0;

int
init_board(void)
{
    int i = 0;
    if (init_usart())
        i |= BOARD_HAS_USART;
    if ((mem_config = init_memory()) != 0)
        i |= BOARD_HAS_MEM_OK;
    if (0 == aceprobe(0))
        i |= BOARD_HAS_DISK0;
    if (0 == aceprobe(1))
        i |= BOARD_HAS_DISK1;
    if (enic_present(0))
        i |= BOARD_HAS_NET;
    board_config = i;
    return i;
}

int getsysid(void)
{
    int systype = XS_ML40x;

    /* If it looks like a duck..
     * ML402: 1 SRAM, 1 DRAM, 0 Ether
     * ML50x: 0 SRAM, 1 DRAM, 1 Ether
     * BEE3:  0 SRAM, 2 DRAM, 1 Ether
     * NB:  mem_config  returns (nfl<<16) | (nsr << 8) | (ndr << 0);
     */
    if (board_config & BOARD_HAS_NET) {
        if ((mem_config & 0xffff) == 2)
            systype = XS_BE3;
        else
            systype = XS_ML50x;
    }

    return MAKESYSID(1,1,systype,MIPS_eMIPS);
}

