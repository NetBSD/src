/*	$NetBSD: mbr_bootsel.S,v 1.3 1999/04/28 23:27:02 fvdl Exp $	*/


/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Copyright (C) 1998 Wolfgang Solfrank.
 * Copyright (C) 1998 TooLs GmbH.
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
 *      This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * i386 master boot code
 */

#include <machine/asm.h>

#define data32  .byte 0x66

/*
 * Gas can't emit 16 bit code. Define each instruction that we need where
 * a 32-bit value (either data or addr) may be inserted in the generated
 * code, or where we would need a prefix but can do without (saving some
 * space).
 */

/*
 * Register values used in the i386 instruction set
 */
#define AL	0
#define BL	3
#define CL	1
#define DL	2
#define AH	4
#define BH	7
#define CH	5
#define DH	6

#define AX	0
#define BX	3
#define CX	1
#define DX	2
#define SP	4
#define BP	5
#define SI	6
#define DI	7

#define SI_INDEX	4
#define DI_INDEX	5
#define BP_INDEX	7


/*
 * .. and the 16 bit instruction defines.
 */
#define call16(label)		.byte 0xe8 ; .word label - . - 2
#define jmp16(label)		.byte 0xe9 ; .word label - . - 2
#define jznear16(label)		.byte 0x0f ; .byte 0x84 ; .word label - . - 2
#define jmpfar(segval,addr)	.byte 0xea ; .word addr ; .word segval

#define movb_mem_al(mem)	.byte 0xa0 ; .word mem
#define movb_al_mem(mem)	.byte 0xa2 ; .word mem

#define movw_mem_ax(mem)	.byte 0xa1 ; .word mem
#define movw_ax_mem(mem)	.byte 0xa3 ; .word mem

#define movw_mem_reg(mem,reg) \
	.byte 0x8b ; .byte (0x06 | ((reg) << 3) ; .word mem

#define movw_imm_reg(imm,reg) \
	.byte 0xb8 | reg ; .word imm

#define movb_ireg0_reg(ireg,reg) .byte 0x8a ; .byte (reg << 3) | ireg
#define movb_iregoff_reg(ireg,off,reg) \
	.byte 0x8a ; .byte 0x40 | (reg << 3) | ireg ; .byte off
#define movw_iregoff_reg(ireg,off,reg) \
	.byte 0x8b ; .byte 0x40 | (reg << 3) | ireg ; .byte off

#define movb_reg_mem(reg,mem) \
	.byte 0x88 ; .byte 0x6 | (reg << 3) ; .word mem
#define movb_mem_reg(mem,reg) \
	.byte 0x8a ; .byte 0x6 | (reg << 3) ; .word mem

#define movl_imm_ireg0(imm,ireg) \
	.byte 0x66 ; .byte 0xc7 ; .byte ireg ; .long imm
#define movl_imm_iregoff(imm,ireg,off) \
	.byte 0x66 ; .byte 0xc7 ; .byte 0x40 | ireg ; .byte off ; .long imm

#define cmp_imm_ax(imm)	.byte 0x3d ; .word imm

#define cmpb_imm_ireg0(imm,reg) \
	.byte 0x80 ; .byte 0x38 + reg ; .byte imm

#define cmpb_imm_iregoff(imm,reg,off) \
	.byte 0x80 ; .byte 0x78 + reg ; .byte off ; .byte imm

#define cmpb_imm_ireg0_bx(imm,reg) \
	.byte 0x80 ; .byte 0x34 + reg ; .byte imm

#define cmpb_mem_reg(mem,reg) .byte 0x3a ; .byte 0x06 | (reg << 3) ; .word mem
#define cmpw_imm_reg(imm,reg) .byte 0x81 ; .byte 0xf8 + reg ; .word imm

#define and_imm_ax(imm)	.byte 0x25 ; .word imm



#define	BOOTADDR	0x7c00

/*
 * Each entry in the boot select table is a nul-terminated string
 * of 8 bytes (not including the 0). A zero-length string (i.e.
 * the first char is 0) indicates an unused entry.
 */
#define PARTNAMESIZE	8
#define TABENTRYSIZE	(PARTNAMESIZE + 1)
#define NAMETABSIZE	4 * TABENTRYSIZE

/*
 * Flag defines. Currently only used to make the boot selector active
 * or inactive. There may never be more because of space constraints.
 */
#define BFL_SELACTIVE	0x01
#define BFL_EXT13	0x02

/*
 * Scan values for the various keys we use, as returned by the BIOS
 */
#define SCAN_ENTER	0x1c
#define SCAN_F1		0x3b
#define SCAN_F5		0x3f
#define SCAN_F10	0x44

/*
 * Minimum and maximum drive number that is considered to be valid.
 */
#define MINDRV		0x80
#define MAXDRV		0x88

/*
 * Error codes. Done this way to save space.
 */
#define ERR_INVPART	'1'		/* Invalid partition table */
#define ERR_READ	'2'		/* Read error */
#define ERR_NOOS	'3'		/* Magic no. check failed for part. */

	.text
/*
 * Move ourselves out of the way first.
 */
ENTRY(start)
	data32
	xorl	%eax, %eax
	movl	%ax, %ss
	movl	$BOOTADDR, %sp
	movl	%ax, %es
	movl	%ax, %ds
	xorl	%si,%si
	movl	%sp, %si
	movw_imm_reg(_C_LABEL(start),DI)
	movw_imm_reg(0x100,CX)
	rep
	movsl
	jmpfar(0,1f)
/*
 * Sanity check the drive number passed by the BIOS. Some BIOSs may not
 * do this and pass garbage.
 */
1:
	testb	$0x78,%dl
	jz 3f
	movb $0x80,%dl
3:
	movb_reg_mem(DL,drvno)
bootsel:
	movb_mem_al(flags)
	testb	$BFL_SELACTIVE,%al
	jznear16(getactive)
/*
 * The bootselector is active. Walk through the selector (name) table,
 * printing used entries.
 */
	movw_imm_reg(nametab,DI)
	movb	$0x31,%al
	movb	$4,%cl
1:
	cmpb_imm_ireg0(0,DI_INDEX)
	jz	2f
	movb_al_mem(prefix+1)
	movw_imm_reg(prefix,SI)
	call16(putasciz)
	movl	%edi,%esi
	call16(putasciz)
	movw_imm_reg(newline,SI);
	call16(putasciz)
2:
	incb	%al
	addl	$TABENTRYSIZE,%di
	loop	1b

/*
 * Get the initial time value for the timeout comparison. It is returned
 * by int 1a in cx:dx. Make sure to grab the whole 32 bit value, otherwise
 * it'd wrap around every hour. Now it'll only do so every 24 hours. Boots
 * should happen infrequent enough that this isn't a problem.
 *
 * Loop around checking for a keypress until we have one, or timeout is
 * reached.
 */
	xorb	%ah,%ah
	int	$0x1a
	data32
	shll	$16,%ecx
	movl	%edx,%edi
	data32
	orl	%ecx,%edi
3:
	movb	$1,%ah
	int	$0x16
	jnz	4f
	xorb	%ah,%ah
	int	$0x1a
	data32
	shll	$16,%ecx
	data32
	orl	%ecx,%edx
	data32
	subl	%edi,%edx
	movw_mem_ax(timeout)
	cmpl	%eax,%edx
	jl	3b
	movb_mem_al(defkey)
	jmp	default
4:
	xorb	%ah,%ah
	int	$0x16
	movb	%ah,%al
default:
/*
 * <enter> -> boot active partition.
 */
	cmpb	$SCAN_ENTER,%al
	je	getactive
/*
 * F1-F4 -> boot partition 1-4
 */
	subb	$SCAN_F1,%al
	cmpb	$9,%al
	jg	4b
	cmpb	$3,%al
	jle	5f
/*
 * F5-F10 -> boot disk 0-5. Check if the requested disk isn't above
 * the number of disks actually in the system as stored in 0:0475 by
 * the BIOS. This is always sector 0, so never use int13 extensions.
 */
	subb	$4,%al
	cmpb_mem_reg(0x0475,AL)
	jge	4b
	movw_imm_reg(fakeent,SI)
	addb	$0x80,%al
	movb	%al,%dl
	jmp16(noext)
5:
/*
 * Check if the requested entry is actually active in the partition and
 * bootmenu table. If not, just do nothing.
 */
	movw_imm_reg(parttab,SI)
	and_imm_ax(0xff)
	movl	%eax,%ebx
	shll	$4,%eax
	add	%eax,%esi
	cmpb_imm_iregoff(0,SI_INDEX,4)
	je	4b
	movw_imm_reg(nametab,DI)
	imul	$TABENTRYSIZE,%ebx
	cmpb_imm_ireg0_bx(0,DI_INDEX)
	je	4b
	jmp	boot

/*
 * Look for the (first) active partition
 */
getactive:
	movw_imm_reg(parttab,SI)
	movw_imm_reg(4,CX)
1:
	cmpb_imm_ireg0(0x80,SI_INDEX)
	jz	boot
	addl	$0x10, %esi
	loop	1b
/*
 * No active partition.
 */
invpart:
	movb	$ERR_INVPART,%al
	jmp	errhang

/*
 * Active partition pointed to by si.
 * Read the first sector.
 *
 * First determine whether we have int13-extensions, by calling
 * int 13, function 41. Check for the magic number returned,
 * and the disk packet capability.
 */
boot:
	movb_mem_reg(drvno,DL)
boot2:
	movb_mem_al(flags)
	testb	$BFL_EXT13,%al
	jz	noext

	push	%esi
	push	%edx
	movw_imm_reg(0x55aa,BX)
	movb	$0x41, %ah
	int	$0x13
	pop	%edx
	pop	%esi
	jc	noext
	cmpw_imm_reg(0xaa55,BX)
	jnz	noext
	testb	$1, %cl
	jz	noext

/*
 * Modify the partition table entry to look like an int13-extension
 * parameter block, so we can feed it to the extended read call.
 * XXX this means that we can only use it once, we can't jump back
 * here if the read fails.
 */
	movl_imm_ireg0(0x10010,SI_INDEX)
	movl_imm_iregoff(BOOTADDR,SI_INDEX,4)
	movl_imm_iregoff(0,SI_INDEX,12)
	movb	$5, %dh
1:
	push	%esi
	push	%edx
	movb	$0x42, %ah
	int	$0x13
	pop	%edx
	pop	%esi
	jnc	ok
	dec	%dh
	jnz	1b
rderr:
	movb	$ERR_READ,%al
errhang:
	movw_imm_reg(err,SI)
	call16(putasciz)
	call16(putc)
hang:
	sti
	jmp 	hang
/*
 * print a 0-terminated ASCII string. Address in si.
 */
putasciz:
	push	%eax
1:
	lodsb
	testb	%al, %al
	jz	2f
	call16(putc)
	jmp	1b
2:
	pop	%eax
	ret
putc:
	movb	$0xe, %ah
	movb	$7, %bl
	int	$0x10
	ret

/*
 * No int13-extensions available, try old the method to load the frirst
 * sector of the partition.
 */
noext:
	movb_iregoff_reg(SI_INDEX,1,DH)
	movw_iregoff_reg(SI_INDEX,2,CX)
load:
	movw_imm_reg(5,DI)
1:
	movw_imm_reg(BOOTADDR,BX)
	movw_imm_reg(0x201,AX)
	push	%edi
	push	%edx
	int	$0x13
	pop	%edx
	pop	%edi
	jnc	ok
	dec	%edi
	jnz	1b
	jmp	rderr

/*
 * Check signature for valid bootcode
 */
ok:
	movw_mem_ax(BOOTADDR+0x1fe)
	cmp_imm_ax(0xaa55)
	jz	1f
	movb	$ERR_NOOS,%al
	jmp	errhang
1:
	jmpfar(0,BOOTADDR)

newline:
	.asciz	"\r\n"
prefix:
	.asciz	"F1: "
err:
	.asciz	"E"
/*
 * Fake partition entry used to boot from other disks. First byte is
 * overloaded, it's also used as storage for the drive number. We're
 * not using that in the entry.
 */
drvno:
fakeent:
	.byte 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00
	.long 0x0000, 0x0001
	. = _C_LABEL(start) + (0x1bc - NAMETABSIZE - 4)
/*
 * Default action, as a keyvalue we'd normally read from the BIOS. This
 * saves a bit of space.
 */
defkey:
	.byte	SCAN_ENTER
flags:
	.byte	0x01
/*
 * Timeout value. 65536 ticks per hour, which is about 18.2 times per second.
 */
timeout:
	.word	0x00b6
/*
 * Space for name/select table and partition table. If DEBUG is defined,
 * use some values from one of my machines, for testing.
 */
nametab:
#ifdef DEBUG
	.asciz	"Windows "
	.asciz	"NetBSD  "
	.byte	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	.byte	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#else
	.fill	NAMETABSIZE,0x01,0x00
#endif
	. = _C_LABEL(start) + 0x1bc
ourmagic:
	.byte	0x55, 0xaa
	. = _C_LABEL(start) + 0x1be
#ifdef DEBUG
parttab:
	.word 0x0100
	.word 0x0001, 0xde06, 0x1f3e, 0x003f, 0x0000, 0xd000, 0x0007, 0xde80
	.word 0x1f3f, 0x24a5, 0xe57e, 0xd03f, 0x0007, 0x1c80, 0x006f, 0x0000
	.word 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
	.word 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
#else
parttab:
	.fill	0x40,0x01,0x00
#endif
	. = _C_LABEL(start) + 0x1fe
	.byte	0x55, 0xaa
