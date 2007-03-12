/*	$NetBSD: mips.c,v 1.3.82.1 2007/03/12 05:48:14 rmind Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura.
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */
#include <pbsdboot.h>

#define DUMMYBUFSIZE	(1024*32)
int dummy_buf[DUMMYBUFSIZE];

int
mips_boot(void *map)
{
	unsigned char *mem;
	unsigned long jump_instruction, phys_mem;
	int i, j;

	/*
	 *  allocate physical memory for startup program.
	 */
	if ((mem = (unsigned char*)vmem_alloc()) == NULL) {
		msg_printf(MSG_ERROR, whoami, TEXT("Can't allocate root page.\n"));
		return -1;
	}

	/*
	 *  copy startup program code
	 */
	memcpy(mem, system_info.si_asmcode, system_info.si_asmcodelen);
  
	/*
	 *  set map address (override target field of asmcode 
	 *  "lui a0, 0x0; ori a0, 0x0;".
	 */
	*(unsigned short*)&mem[0] = (unsigned short)(((long)map) >> 16);
	*(unsigned short*)&mem[4] = (unsigned short)map;

	/*
	 *  construct start instruction
	 */
	if (!(phys_mem = (unsigned long)vtophysaddr(mem))) {
		msg_printf(MSG_ERROR, whoami, TEXT("Missing physical page.\n"));
		return -1;
	}
	jump_instruction = (0x08000000 | ((phys_mem >> 2) & 0x03ffffff));

	/*
	 *  map interrupt vector
	 */
	mem = (unsigned char*)VirtualAlloc(0, 0x400, MEM_RESERVE, PAGE_NOACCESS);
	if (!(VirtualCopy((LPVOID)mem, (LPVOID)(system_info.si_dramstart >> 8), 0x400,
			  PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL))) {
		msg_printf(MSG_ERROR, whoami, TEXT("Mapping interrupt vector failed.\n"));
		return -1;
	}

	/*
	 *  Flush D-cache
	 */
	for (i = 0; i < DUMMYBUFSIZE; i++) {
		dummy_buf[i] = j;
	}
	for (i = 0; i < DUMMYBUFSIZE; i++) {
		j = dummy_buf[i];
		j++;
	}
	/*	
	 *  Flush I-cache
	 */
	__asm(".space 32768;");

	/*
	 *  GO !
	 */
	*(unsigned long*)&mem[system_info.si_intrvec] = jump_instruction;

	return (0); /* not reachable */
}
