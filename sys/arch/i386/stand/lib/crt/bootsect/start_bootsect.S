/*	$NetBSD: start_bootsect.S,v 1.8 1999/01/22 22:43:44 thorpej Exp $	*/
	
/*
 * BIOS bootsector startup
 * parts from netbsd:sys/arch/i386/boot/start.S
 * and freebsd:sys/i386/boot/biosboot/boot2.S
 */

/*
 * Ported to boot 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 *
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
  Copyright 1988, 1989, 1990, 1991, 1992 
   by Intel Corporation, Santa Clara, California.

                All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <machine/asm.h>
#define	addr32	.byte 0x67
#define	data32	.byte 0x66
#include <machine/disklabel.h>

#include "bbinfo.h"

BOOTSEG		=	0x0100	# boot will be loaded here (below 640K)
BOOTSTACK	=	0xfffc	# boot stack
LOADSZ		=	PRIM_LOADSZ	# size of first loaded chunk

SIGNATURE		=	0xaa55
PARTSTART		=	DOSPARTOFF	# starting address of partition table
NUMPART		=	NDOSPART	# number of partitions in partition table
PARTSZ		=	16	# sizeof(struct dos_partition)
NETBSDPART	=	DOSPTYP_NETBSD
#ifdef COMPAT_386BSD_MBRPART
OLDBSDPART	= 	DOSPTYP_386BSD
#endif
BOOTABLE		=	0x80	# value of dp_flag, means bootable partition

	.text
_C_LABEL(boot1):
ENTRY(start)

	/*
	 * XXX I have encountered at least one machine (a no-name laptop
	 * with an AMI WinBIOS) that will refuse to run the bootblock
	 * unless this short jump and nop are here. I'm not certain, but
	 * this may be a case of the BIOS performing some kind of simple
	 * virus detection.
	 */
	jmp pacify_braindead_bios
	nop
pacify_braindead_bios:

	# start (aka boot1) is loaded at 0x0:0x7c00 but we want 0x7c0:0
	# ljmp to the next instruction to adjust %cs
	data32
	ljmp $0x7c0, $start1

	/* some BIOSs insist on patching the BPB, esp. on LS120 drives */
	. = _C_LABEL(boot1) + 0x25

start1:
	# set up %ds
	movl	%cs, %ax
	movl	%ax, %ds

	# set up %ss and %esp
	data32
	movl	$BOOTSEG, %eax
	movl	%ax, %ss
	data32
	movl	$BOOTSTACK, %esp

	/*** set up %es, (where we will load boot2 to) ***/
	movl	%ax, %es

	# bootstrap passes us drive number in %dl
	cmpb	$0x80, %dl
	data32
	jae	hd

fd:
#	reset the disk system
	movb	$0x00, %ah
	int	$0x13
	data32
	movl	$0x0001, %ecx	# cyl 0, sector 1
	movb	$0x00, %dh	# head
	# XXX Override the drive number.
	movb	$0x00, %dl
	data32
	jmp	load

hd:	/**** load sector 0 (DOSBBSECTOR) into the BOOTSEG ****/
	data32
	movl	$0x0201, %eax
	xorl	%ebx, %ebx	# %bx = 0
	data32
	movl	$0x0001, %ecx
	data32
	andl	$0xff, %edx
	/*mov	$0x0080, %edx*/
	int	$0x13
	data32
	jb	read_error

	/***# find the first NetBSD partition *****/
	data32
	movl	$PARTSTART, %ebx
	data32
	movl	$NUMPART, %ecx
again:
	addr32
	movb    %es:4(%ebx), %al	/* dp_typ */
	cmpb	$NETBSDPART, %al
	data32
	je	found
	data32
	addl	$PARTSZ, %ebx
	data32
	loop	again

#ifdef COMPAT_386BSD_MBRPART
	/* didn't find a NetBSD partition, look for a 386BSD one */
	data32
	movl	$PARTSTART, %ebx
	data32
	movl	$NUMPART, %ecx
again1:
	addr32
	movb    %es:4(%ebx), %al	/* dp_typ */
	cmpb	$OLDBSDPART, %al
	data32
	je	found
	data32
	addl	$PARTSZ, %ebx
	data32
	loop	again1
#endif

	/* didn't find either NetBSD or 386BSD partitions */
	data32
	movl	$enoboot, %esi
	data32
	jmp	err_stop


found:
/*
 * First we check for int13 extensions.
 * If they work, let's use them.
 * If not, revert to the traditional way of doing things.
 */

/*
 * BIOS call "INT 0x13 Function 0x41" to check for int13 extensions.
 *	Call with	%ah = 0x41
 *			%bx = 0x55aa
 *			%dl = drive (0x80 for hard disk)
 *	Return:
 *			%cf = clear on success, set on error
 *			%bx = 0xaa55
 *			%cx = API subset bitmap, &1 -> extended disk access
 */
	pushl	%ebx		/* save partition entry */
	pushl	%edx		/* save drive number */
	movb	$0x41, %ah
	data32
	movl	$0x55aa, %bx
	int	$0x13
	popl	%edx
	movl	%ebx, %eax
	popl	%ebx
	data32
	jb	tradint13
	data32
	movl	$0xaa55, %edi
	cmpl	%edi, %eax
	jnz	tradint13
	testb	$1, %cl
	data32
	jz	tradint13

/*
 * BIOS call "INT 0x13 Function 0x42", extended read
 *	Call with	%ah = 0x42
 *			%dl = drive (0x80 for hard disk)
 *			%ds:%si = parameter:
 *				0: 0x10 (size of packet)
 *				1: 0
 *				2: number of sectors
 *				4: segment:offset of buffer
 *				8: block number (8 bytes)
 */
	pushl	%ss
	popl	%ds
	data32
	pushl	$0		/* high 4 bytes of sector = 0 */
	data32
	addr32
	movl	%es:8(%ebx), %eax	/* low 4 bytes of sector */
	data32
	pushl	%eax
	movl	%es, %ax
	data32
	shll	$16, %eax	/* shifts 0 from right */
	data32
	pushl	%eax		/* so we get address 0 in BOOTSEG */
	data32
	pushl	$0x10+LOADSZ*0x10000
	movl	%esp, %esi
	movb	$0x42, %ah
	int	$0x13
	pushl	%cs		/* don't bother adjustin %esp, */
	popl	%ds		/* but reestablish %ds */
	data32
	jb	read_error
	data32
	jmp	to_boot2

/*
 * BIOS call "INT 0x13 Function 0x2" to read sectors from disk into memory
 *	Call with       %ah = 0x2
 *			%al = number of sectors
 *			%ch = cylinder
 *			%cl = sector
 *			%dh = head
 *			%dl = drive (0x80 for hard disk, 0x0 for floppy disk)
 *			%es:%bx = segment:offset of buffer
 *	Return:
 *			%al = 0x0 on success; err code on failure
 */
tradint13:

	addr32
	movb	%es:1(%ebx), %dh /* head: dp_sdh */
	addr32
	movl	%es:2(%ebx), %ecx /*sect, cyl: dp_ssect + (dp_scyl << 8)*/

load:
	movb	$0x2, %ah	/* function 2 */
	movb	$LOADSZ, %al	/* number of blocks */
	xorl	%ebx, %ebx	/* %bx = 0, put it at 0 in the BOOTSEG */
	int	$0x13
	data32
	jb	read_error

to_boot2:
	# ljmp to the second stage boot loader (boot2).
	# After ljmp, %cs is BOOTSEG and boot1 (512 bytes) will be used
	# as an internal buffer "intbuf".

	data32
	ljmp	$BOOTSEG, $_C_LABEL(boot2)

/*
 * read_error
 */
read_error:
	data32
	movl	$eread, %esi
err_stop:
	data32
	call	message
	cli
	hlt

/*
 * message: write the error message in %ds:%esi to the console
 */
message:
/*
 * BIOS call "INT 10H Function 0Eh" to write character to console
 *	Call with	%ah = 0x0e
 *			%al = character
 *			%bh = page
 *			%bl = foreground color
 */
	data32
	pushl	%eax
	data32
	pushl	%ebx
	data32
	pushl	%edx

nextb:
	cld
	lodsb			# load a byte into %al
	testb	%al, %al
	data32
	jz	done

	movb	$0x0e, %ah
	data32
	movl	$0x0001, %ebx
	int	$0x10

	data32
	jmp	nextb

done:
	data32
	popl	%edx
	data32
	popl	%ebx
	data32
	popl	%eax
	data32
	ret

eread:	.asciz		"Read error\r\n"
enoboot: .asciz		"No bootable partition\r\n"

/* throw in a partition in case we are block0 as well */
/* flag, head, sec, cyl, typ, ehead, esect, ecyl, start, len */
	. = _C_LABEL(boot1) + PARTSTART
	.byte	0x0,0,0,0,0,0,0,0
	.long	0,0
	.byte	0x0,0,0,0,0,0,0,0
	.long	0,0
	.byte	0x0,0,0,0,0,0,0,0
	.long	0,0
	.byte	BOOTABLE,0,1,0,NETBSDPART,0,16,0
	.long	0,16
/* the last 2 bytes in the sector 0 contain the signature */
	. = _C_LABEL(boot1) + 0x1fe
	.short	SIGNATURE
	. = _C_LABEL(boot1) + 0x200
	.globl	_C_LABEL(disklabel)
_C_LABEL(disklabel):
	. = _C_LABEL(boot1) + 0x400

ENTRY(ourseg)
	.long	0

/**************************************************************************
GLOBAL DESCRIPTOR TABLE
**************************************************************************/
	.align	4
gdt:
	.word	0, 0
	.byte	0, 0x00, 0x00, 0

#ifdef SUPPORT_LINUX	/* additional dummy */
	.word	0, 0
	.byte	0, 0x00, 0x00, 0
#endif

	/* kernel code segment */
	.globl flatcodeseg
flatcodeseg = . - gdt
	.word	0xffff, 0
	.byte	0, 0x9f, 0xcf, 0

	/* kernel data segment */
	.globl flatdataseg
flatdataseg = . - gdt
	.word	0xffff, 0
	.byte	0, 0x93, 0xcf, 0

	/* boot code segment, will be patched */
bootcodeseg = . - gdt
	.word	0xffff, 0
	.byte	0, 0x9e, 0x40, 0

	/* boot data segment, will be patched */
bootdataseg = . - gdt
	.word	0xffff, 0
	.byte	0, 0x92, 0x0f, 0

	/* 16 bit real mode, will be patched */
bootrealseg = . - gdt
	.word	0xffff, 0
	.byte	0, 0x9e, 0x00, 0
gdtlen = . - gdt

	.align	4
gdtarg:
	.word	gdtlen-1		/* limit */
	.long	0			/* addr, will be inserted */

_C_LABEL(boot2):
	data32
	xorl	%eax, %eax
	movl	%cs, %ax
	movl	%ax, %ds
	movl	%ax, %es
	addr32
	data32
	movl	%eax, _C_LABEL(ourseg)
	data32
	shll	$4, %eax

	/* fix up GDT entries for bootstrap */
#define FIXUP(gdt_index) \
	addr32; \
	movl	%eax, gdt+gdt_index+2;	/* actually movw %ax */ \
	addr32; \
	movb	%bl, gdt+gdt_index+4

	data32
	shldl	$16, %eax, %ebx

	FIXUP(bootcodeseg)
	FIXUP(bootrealseg)
	FIXUP(bootdataseg)

	/* fix up GDT pointer */
	data32
	addl	$gdt, %eax
	addr32
	data32
	movl	%eax, gdtarg+2

	/* change to protected mode */
	data32
	call	_C_LABEL(real_to_prot)

	movzbl	%dl, %edx	/* discard head (%dh) and random high bits */
	pushl	%edx
	call	_C_LABEL(bootsectmain)
ENTRY(exit)
	cli
	hlt

CR0_PE		=	0x1

/*
 * real_to_prot()
 * 	transfer from real mode to protected mode.
 */
ENTRY(real_to_prot)
	# guarantee that interrupt is disabled when in prot mode
	cli

	# load the gdtr
	addr32
	data32
	lgdt	gdtarg

	# set the PE bit of CR0
	movl	%cr0, %eax

	data32
	orl	$CR0_PE, %eax
	movl	%eax, %cr0 

	# make intrasegment jump to flush the processor pipeline and
	# reload CS register
	data32
	ljmp	$bootcodeseg, $xprot

xprot:
	# we are in USE32 mode now
	# set up the protected mode segment registers : DS, SS, ES
	movl	$bootdataseg, %eax
	movl	%ax, %ds
	movl	%ax, %ss
	movl	%ax, %es

	ret

/*
 * prot_to_real()
 * 	transfer from protected mode to real mode
 */
ENTRY(prot_to_real)
	# set up a dummy stack frame for the second seg change.
	# Adjust the intersegment jump instruction following 
	# the clearing of protected mode bit.
	# This is self-modifying code, but we need a writable
	# code segment, and an intersegment return does not give us that.

	movl	_C_LABEL(ourseg), %eax
	movw	%ax, xreal-2

	# Change to use16 mode.
	ljmp	$bootrealseg, $x16

x16:
	# clear the PE bit of CR0
	movl	%cr0, %eax
	data32
	andl 	$~CR0_PE, %eax
	movl	%eax, %cr0
	# Here we have an 16 bits intersegment jump.
	.byte 0xea
	.word xreal
	.word 0

xreal:
	# we are in real mode now
	# set up the real mode segment registers : DS, SS, ES
	movl	%cs, %ax
	movl	%ax, %ds
	movl	%ax, %ss
	movl	%ax, %es

	sti
	data32
	ret

/*
 * pbzero(dst, cnt)
 *	where dst is a physical address and cnt is the length
 */
ENTRY(pbzero)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%es
	pushl	%edi

	cld

	# set %es to point at the flat segment
	movl	$flatdataseg, %eax
	movl	%ax, %es

	movl	8(%ebp), %edi		# destination
	movl	12(%ebp), %ecx		# count
	xorl	%eax, %eax		# value

	rep
	stosb

	popl	%edi
	popl	%es
	popl	%ebp
	ret

/*
 * vpbcopy(src, dst, cnt)
 *	where src is a virtual address and dst is a physical address
 */
ENTRY(vpbcopy)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%es
	pushl	%esi
	pushl	%edi

	cld

	# set %es to point at the flat segment
	movl	$flatdataseg, %eax
	movl	%ax, %es

	movl	8(%ebp), %esi		# source
	movl	12(%ebp), %edi		# destination
	movl	16(%ebp), %ecx		# count

	rep
	movsb

	popl	%edi
	popl	%esi
	popl	%es
	popl	%ebp
	ret

/*
 * pvbcopy(src, dst, cnt)
 *	where src is a physical address and dst is a virtual address
 */
ENTRY(pvbcopy)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ds
	pushl	%esi
	pushl	%edi

	cld

	# set %ds to point at the flat segment
	movl	$flatdataseg, %eax
	movl	%ax, %ds

	movl	8(%ebp), %esi		# source
	movl	12(%ebp), %edi		# destination
	movl	16(%ebp), %ecx		# count

	rep
	movsb

	popl	%edi
	popl	%esi
	popl	%ds
	popl	%ebp
	ret

ENTRY(vtophys)
	movl _C_LABEL(ourseg), %eax
	shll	$4, %eax
	addl 4(%esp), %eax
	ret
