/*	$NetBSD: arcbios_stubs.S,v 1.1 2020/08/14 16:53:06 skrll Exp $	*/

/*-
 * Copyright (c) 1996 M. Warner Losh.  All rights reserved.
 * Copyright (c) 1996, 1997, 1998 Per Fogelstrom.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 */

#include <machine/asm.h>

/*
 *	ARC Bios trampoline code.
 *	Note we have to save/restore reserved MIPS_CURLWP register.
 */
#define ARC_Call(Name,Offset)				\
	.text						;\
	.ent	Name					;\
	.align	3					;\
	.set	noreorder				;\
	.globl	Name	 				;\
Name:							;\
	subu	$29, CALLFRAME_SIZ			;\
	sw	$31, CALLFRAME_RA($29)			;\
	sw	$16, CALLFRAME_SP($29)			;\
	lw	$2, 0x80001020				;\
	lw	$2, Offset($2)				;\
	jalr	$2					;\
	 move	$16, MIPS_CURLWP			;\
	move	MIPS_CURLWP, $16			;\
	lw	$31, CALLFRAME_RA($29)			;\
	lw	$16, CALLFRAME_SP($29)			;\
	j	$31					;\
	 addu	$29, CALLFRAME_SIZ	 		;\
	.end	Name

#define ARC_Call5(Name,Offset)				\
	.text						;\
	.ent	 #Name 					;\
	.align	3					;\
	.set	noreorder				;\
	.globl	Name 					;\
Name:							;\
	subu	$29, CALLFRAME_SIZ + 4		 	;\
	sw	$31, CALLFRAME_RA + 4($29)		;\
	sw	$16, CALLFRAME_SP + 4($29)		;\
	lw	$12, CALLFRAME_SIZ + 4 + 16($29)	;\
	sw	$12, 16($29)				;\
	lw	$2, 0x80001020				;\
	lw	$2, Offset($2)				;\
	jalr	$2					;\
	 move	$16, MIPS_CURLWP	 		;\
	move	MIPS_CURLWP, $16			;\
	lw	$31, CALLFRAME_RA + 4($29)		;\
	lw	$16, CALLFRAME_SP + 4($29)		;\
	j	$31					;\
	 addu	$29, CALLFRAME_SIZ + 4		 	;\
	.end	Name

ARC_Call(Bios_Load,			0x00);
ARC_Call5(Bios_Invoke,			0x04);
ARC_Call(Bios_Execute,			0x08);
ARC_Call(Bios_Halt,			0x0c);
ARC_Call(Bios_PowerDown,		0x10);
ARC_Call(Bios_Restart,			0x14);
ARC_Call(Bios_Reboot,			0x18);
ARC_Call(Bios_EnterInteractiveMode,	0x1c);
ARC_Call(Bios_Unused1,			0x20);	/* return_from_main? */
ARC_Call(Bios_GetPeer,			0x24);
ARC_Call(Bios_GetChild,			0x28);
ARC_Call(Bios_GetParent,		0x2c);
ARC_Call(Bios_GetConfigurationData,	0x30);
ARC_Call(Bios_AddChild,			0x34);
ARC_Call(Bios_DeleteComponent,		0x38);
ARC_Call(Bios_GetComponent,		0x3c);
ARC_Call(Bios_SaveConfiguration,	0x40);
ARC_Call(Bios_GetSystemId,		0x44);
ARC_Call(Bios_GetMemoryDescriptor,	0x48);
ARC_Call(Bios_Unused2,			0x4c);	/* signal??? */
ARC_Call(Bios_GetTime,			0x50);
ARC_Call(Bios_GetRelativeTime,		0x54);
ARC_Call(Bios_GetDirectoryEntry,	0x58);
ARC_Call(Bios_Open,			0x5c);
ARC_Call(Bios_Close,			0x60);
ARC_Call(Bios_Read,			0x64);
ARC_Call(Bios_GetReadStatus,		0x68);
ARC_Call(Bios_Write,			0x6c);
ARC_Call(Bios_Seek,			0x70);
ARC_Call(Bios_Mount,			0x74);
ARC_Call(Bios_GetEnvironmentVariable,	0x78);
ARC_Call(Bios_SetEnvironmentVariable,	0x7c);
ARC_Call(Bios_GetFileInformation,	0x80);
ARC_Call(Bios_SetFileInformation,	0x84);
ARC_Call(Bios_FlushAllCaches,		0x88);
/* note: the followings don't exist on SGI */
#ifdef arc
ARC_Call(Bios_TestUnicodeCharacter,	0x8c);
ARC_Call(Bios_GetDisplayStatus,		0x90);
#endif

