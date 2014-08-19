/*	$NetBSD: riscoscalls.S,v 1.4.106.1 2014/08/20 00:02:40 tls Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
 * Copyright (c) 2002 Reinoud Zandijk
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

#include <machine/asm.h>
#include <riscoscalls.h>

ENTRY(os_writec)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	OS_WriteC
	ldmdb	fp, {fp, sp, pc}

ENTRY(os_new_line)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	OS_NewLine
	ldmdb	fp, {fp, sp, pc}

ENTRY(os_readc)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	OS_ReadC
	ldmdb	fp, {fp, sp, pc}

ENTRY(os_cli)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	OS_CLI
	ldmdb	fp, {fp, sp, pc}

ENTRY(xos_cli)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	XOS_CLI
	movvc	r0, #0
	ldmdb	fp, {fp, sp, pc}

ENTRY(os_byte)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	OS_Byte	
	teq	r3, #0
	strne	r1, [r3]
	ldr	r3, [fp, #4]
	teq	r3, #0
	strne	r2, [r3]
	ldmdb	fp, {fp, sp, pc}

ENTRY(os_word)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	OS_Word
	ldmdb	fp, {fp, sp, pc}

ENTRY(xosargs_read)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r3, r2
	swi	OS_Args
	ldmdbvs	fp, {fp, sp, pc}
	teq	r3, #0
	strne	r2, [r3]
	mov	r0, #0
	ldmdb	fp, {fp, sp, pc}

ENTRY(xosargs_set)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	OS_Args
	movvc	r0, #0
	ldmdb	fp, {fp, sp, pc}

ENTRY(xosgbpb_write)
	mov	ip, sp
	stmfd	sp!, {r4, fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	ip, r3
	mov	r3, r2
	mov	r2, r1
	mov	r1, r0
	mov	r0, #OSGBPB_Write
	swi	XOS_GBPB
	teq	ip, #0
	strne	r3, [ip]
	movvc	r0, #0
	ldmdb	fp, {r4, fp, sp, pc}

ENTRY(xosgbpb_read)
	mov	ip, sp
	stmfd	sp!, {r4, fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	ip, r3
	mov	r3, r2
	mov	r2, r1
	mov	r1, r0
	mov	r0, #OSGBPB_Read
	swi	XOS_GBPB
	teq	ip, #0
	strne	r3, [ip]
	movvc	r0, #0
	ldmdb	fp, {r4, fp, sp, pc}

ENTRY(xosfind_close)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r1, r0
	mov	r0, #OSFind_Close
	swi	XOS_Find
	movvc	r0, #0
	ldmdb	fp, {fp, sp, pc}	

ENTRY(xosfind_open)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	XOS_Find
	ldmdbvs	fp, {fp, sp, pc}
	teq	r3, #0
	strne	r0, [r3]
	mov	r0, #0
	ldmdb	fp, {fp, sp, pc}	

ENTRY(os_get_env)
	mov	ip, sp
	stmfd	sp!, {r4, fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r3, r0
	mov	r4, r1
	swi	OS_GetEnv
	teq	r3, #0
	strne	r1, [r3]
	teq	r4, #0
	strne	r2, [r4]
	ldmdb	fp, {r4, fp, sp, pc}

ENTRY(os_exit)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r2, r1
	ldr	r1, Labex
	swi	OS_Exit
	ldmdb	fp, {fp, sp, pc}
Labex:
	.ascii	"ABEX"

ENTRY(os_int_off)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	OS_IntOff
	ldmdb	fp, {fp, sp, pc}

ENTRY(os_enter_os)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	OS_EnterOS
	ldmdb	fp, {fp, sp, pc}

ENTRY(xosfscontrol_shutdown)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r0, #OSFSControl_Shutdown
	swi	XOS_FSControl
	movvc	r0, #0
	ldmdb	fp, {fp, sp, pc}	

ENTRY(service_pre_reset)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r1, #Service_PreReset
	swi	OS_ServiceCall
	ldmdb	fp, {fp, sp, pc}	

ENTRY(os_read_vdu_variables)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	OS_ReadVduVariables
	ldmdb	fp, {fp, sp, pc}	

ENTRY(xos_swi_number_from_string)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r2, r1
	mov	r1, r0
	swi	XOS_SWINumberFromString
	ldmdbvs	fp, {fp, sp, pc}
	str	r0, [r2]
	mov	r0, #0
	ldmdb	fp, {fp, sp, pc}
	
ENTRY(os_read_mem_map_info)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r2, r0
	mov	r3, r1
	swi	OS_ReadMemMapInfo
	teq	r2, #0
	strne	r0, [r2]
	teq	r3, #0
	strne	r1, [r3]
	ldmdb	fp, {fp, sp, pc}	

ENTRY(os_readsysinfo)
	mov	ip, sp
	stmfd	sp!, {r4-r9, fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r9, r5
	mov	r8, r4
	mov	r7, r3
	mov	r6, r2
	mov	r5, r1
	swi	OS_ReadSysInfo
	teq	r5, #0
	strne	r0, [r5]
	teq	r6, #0
	strne	r1, [r6]
	teq	r7, #0
	strne	r2, [r7]
	ldr	r1, [ip, #0]
	teq	r1, #0
	strne	r3, [r1]
	ldr	r1, [ip, #4]
	teq	r1, #0
	strne	r4, [r1]
	ldmdb	fp, {r4-r9, fp, sp, pc}

ENTRY(os_read_mem_map_entries)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	OS_ReadMemMapEntries
	ldmdb	fp, {fp, sp, pc}

ENTRY(osmemory_read_arrangement_table_size)
	mov	ip, sp
	stmfd	sp!, {r4, fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r3, r0
	mov	r4, r1
	mov	r0, #OSMemory_ReadArrangementTableSize
	swi	OS_Memory
	cmp	r3, #0
	strne	r1, [r3]
	cmp	r4, #0
	strne	r2, [r4]
	ldmdb	fp, {r4, fp, sp, pc}

ENTRY(osmemory_read_arrangement_table)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r1, r0
	mov	r0, #OSMemory_ReadArrangementTable
	swi	OS_Memory
	ldmdb	fp, {fp, sp, pc}

ENTRY(osmemory_page_op)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r0, r0, #OSMemory_PageOp
	swi	OS_Memory
	ldmdb	fp, {fp, sp, pc}

ENTRY(xcache_control)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	swi	XCache_Control
	ldmdbvs	fp, {fp, sp, pc}
	teq	r2, #0
	strne	r0, [r2]
	mov	r0, #0
	ldmdb	fp, {fp, sp, pc}
