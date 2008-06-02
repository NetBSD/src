/*	$NetBSD: fatboot.S,v 1.2.48.1 2008/06/02 13:22:18 mjf Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Laight.
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

/*
 * i386 partition boot code
 * This version reads boot directly from a FAT16 filesystem on LBA
 * Addressable media - eg USB media.
 *
 * The code is read to address 0:7c00 by the mbr code (in sector zero).
 *
 * We assume that the partition contains a 'boot parameter block' that
 * correctly identifies the filesystem.
 *
 * On entry and exit the BIOS drive number is in %dl.
 */

#include <machine/asm.h>
#include <sys/bootblock.h>

#ifndef FAT_ENTRY_SIZE
#error FAT_ENTRY_SIZE not defined
#endif

/* Support for FAT32 could be added - but hasn't been yet. */
#if FAT_ENTRY_SIZE != 16
#error Unsupported FAT_ENTRY_SIZE value
#endif

#define PBR_AFTERBPB	62		/* BPB size in floppy master BR */

#ifdef TERSE_ERROR
/*
 * Error codes. Done this way to save space.
 */
#define ERR_READ		'R'	/* Read error */
#define ERR_NO_BOOT		'B'	/* No /boot */
#define ERR_NOT_FAT16		'F'	/* Not a FAT16 filesystem */
#define	ERR_NO_BOOT_MAGIC_2	'M'	/* No magic in loaded /boot */

#define	set_err(err)	movb	$err, %al

#else
#define	set_err(err)	mov	$err, %ax
#endif

	.text
	.code16
ENTRY(start)
	jmp	start0
	nop
oem_name:               .ascii	"NetBSD40"	/* 8 bytes */
/* FAT16 BIOS/BOOT Parameter Block - see struct mbr_bpbFAT16 in bootblock.h */
#define	bpb_bytes_per_sec   /* .word 0	*/ 0x0b /* bytes per sector */
#define	bpb_sec_per_clust   /* .byte 0	*/ 0x0d /* sectors per cluster */
#define	bpb_res_sectors     /* .word 0	*/ 0x0e /* number of reserved sectors */
#define	bpb_FATs            /* .byte 0	*/ 0x10 /* number of FATs */
#define	bpb_root_dir_ents   /* .word 0	*/ 0x11 /* number of root dir entries */
#define	bpb_sectors         /* .word 0	*/ 0x13 /* total number of sectors */
#define	bpb_media           /* .byte 0	*/ 0x15 /* media descriptor */
#define	bpb_FAT_secs        /* .word 0	*/ 0x16 /* number of sectors per FAT */
#define	bpb_sec_per_track   /* .word 0	*/ 0x18 /* sectors per track */
#define	bpb_heads           /* .word 0	*/ 0x1a /* number of heads */
#define	bpb_hidden_secs     /* .long 0	*/ 0x1c /* # of hidden sectors */
#define	bpb_huge_sectors    /* .long 0	*/ 0x20 /* # of sectors if !bpbSectors*/
/* Extended boot area */
#define	bs_drive_number     /* .byte 0	*/ 0x24 /* but we believe the BIOS ! */
#define	bs_reserved_1       /* .byte 0	*/ 0x25 /* */
#define	bs_boot_sig         /* .byte 0	*/ 0x26 /* */
#define	bs_volume_id        /* .long 0	*/ 0x27 /* Volume ID number */
#define	bs_volume_label     /* .space 11*/ 0x2b /* Volume label */
#define	bs_file_sys_type    /* .space 8	*/ 0x36 /* "FAT16   " */

/* Some locals overlaying the end of the above */
#define sec_p_cl_w	0x2c			/* 16bit bpb_sec_per_clust */
#define	fat_sector	0x30			/* start of FAT in sectors */

	. = start + PBR_AFTERBPB	/* skip BPB */
start0:
	xor	%eax, %eax		/* don't trust values of ds, es or ss */
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss
	mov	$start, %sp
	mov	%sp, %bp		/* to access the pbp */
	push	%dx			/* save drive at -2(%bp) */

	set_err(ERR_NOT_FAT16)
	cmpl	$'A'|'T'<<8|'1'<<16|'6'<<24, bs_file_sys_type+1(%bp)
	jne	error

/* Add 'reserved' (inside ptn) to 'hidden' (ptn offset) */
	mov	bpb_res_sectors(%bp), %ax
	addl	bpb_hidden_secs(%bp), %eax
	mov	%eax, fat_sector(%bp)	/* To get first sector of FAT */

/* Determine base of root directory */
	movzbw	bpb_FATs(%bp), %ax	/* Count of FATs */
	mulw	bpb_FAT_secs(%bp)	/* FAT size in %dx:%ax */
	shl	$16,%edx
	xchg	%ax,%dx			/* FAT size now in %edx */
	addl	fat_sector(%bp), %edx	/* Directory is after FATs */
	push	%cs			/* %cs is zero */
	push	%cs			/* 64-bit for LBA read */
	pushl	%edx			/* Sector number of root dir */

	push	$0x1000			/* Read to 0x10000:0 */
	pop	%es			/* Which we need in %es later */
	push	%es
	push	%cs			/* Offset zero */

/* Convert the root directory size to sectors */
	push	%dx
	mov	bpb_root_dir_ents(%bp), %ax
	mov	$0x20, %dx
	mul	%dx
	divw	bpb_bytes_per_sec(%bp)
	add	$0xffff, %dx		/* Set carry if remainder non-zero */
	adc	$0, %ax			/* and round up the division */
	pop	%dx

/* Read in the entire root directory */
	push	%ax			/* Sectors in root directory */
	cwtl
	addl	%eax, %edx		/* %edx now sector of first file */
	call	read_lba		/* Read entire directory */

/* Scan directory for our file */
	xor	%di, %di
scan_dir:
	mov	$boot_filename, %si
	mov	$11, %cx
	repz cmpsb
	je	found_boot
	or	$31,%di
	inc	%di
	cmp	%ch, %es:(%di)		/* %ch is zero - test end of dir */
	jz	1f
	decw	bpb_root_dir_ents(%bp)
	jnz	scan_dir
1:	set_err(ERR_NO_BOOT)

error:
#ifdef TERSE_ERROR
	movb	%al, errcod
	movw	$errtxt, %si
	call	message
#else
	push	%ax
	movw	$errtxt, %si
	call	message
	pop	%si
	call	message
	movw	$newline, %si
	call	message
#endif
1:	sti
	hlt
	jmp	1b

found_boot:
	movzbl	bpb_sec_per_clust(%bp), %eax
	movw	%ax, sec_p_cl_w(%bp)
	add	%ax, %ax
	subl	%eax, %edx		/* 1st file sector is cluster 2 */
1:	inc	%cl			/* Convert power of 2 ... */
	shr	$1, %ax			/* ... to shift */
	jnz	1b
	dec %cx
	dec %cx
	movw	%es:15(%di), %ax	/* Cluster number for file start */
	push	%es			/* We increment the 'segment' ... */
	pop	%di			/* ... after each read, offset is 0 */

read_data_block:
	mov	%ax, %bx		/* Save cluster number */
	shl	%cl, %eax		/* Convert to sector number */
	add	%eax, %edx
	pushl	%edx			/* Sector to read */
	sub	%eax, %edx		/* Recover base segment */
	push	%di			/* Target address segment! */
	push	$0
	push	sec_p_cl_w(%bp)
	call	read_lba		/* Read a cluster */

/* Update read ptr for next cluster */
	mov	bpb_bytes_per_sec(%bp), %ax
	shr	$4, %ax			/* x86 segment count */
	shl	%cl, %ax		/* for a cluster */
	add	%ax, %di

/* Lookup FAT slot number in FAT table */
	mov	%bx, %ax		/* Recover cluster number */
	push	%dx
	xor	%dx, %dx
	divw	bpb_bytes_per_sec(%bp)
	mov	%dx, %bx		/* Entry in FAT block */
	pop	%dx
	cmp	%ax, fat_cache
	je	lookup_fat

/* We must read a different chuck of the FAT */
	mov	%ax, fat_cache
	cwtl
	shl	$1, %ax
	addl	fat_sector(%bp), %eax
	push	%eax
	push	%ds
	push	$fat_buffer
	push	$2			/* Always read 2 sectors of FAT */
	call	read_lba

/* Now use low part of cluster number to index FAT sector */
lookup_fat:
	add	%bx, %bx		/* 2 bytes per entry... */
	movzwl	fat_buffer(%bx), %eax	/* Next FAT slot */
	cmp	$0xfff0, %ax
	jb	read_data_block

/* Found end of FAT chain - must be EOF  - leap into loaded code */
	mov	$0x1000, %ax
	mov	%ax, %es
	cmpl	$X86_BOOT_MAGIC_2, %es:4
	je	magic_ok
	set_err(ERR_NO_BOOT_MAGIC_2)
err1:	jmp	error

/* Set parameters expected by /boot */
magic_ok:
	mov	bpb_hidden_secs(%bp), %ebx	/* ptn base sector */
	movb	-2(%bp), %dl		/* disk number (could pop %dx) */
	mov	$boot_params + 4, %si
	push	%es
	push	$0
	lret

/* Read disk using on-stack int13-extension parameter block */
read_lba:
	pop	%ax			/* Save rtn addr */
	pushw	$16			/* Stack ctl block length */
	mov	%sp, %si		/* Address ctl block */
	push	%ax			/* restack rtn addr */
	pushal				/* Save everything except %si and %ax*/
	mov	-2(%bp), %dl		/* Disk # saved on entry */
	movb	$0x42, %ah
	int	$0x13
	popal

	set_err(ERR_READ)
	jc	err1
	ret	$12			/* Discard all except high LBA zeros */

/*
 * I hate #including source files, but pbr_magic below has to be at
 * the correct absolute address.
 * Clearly this could be done with a linker script.
 */

#include <message.S>
#if 0
#include <dump_eax.S>
#endif

errtxt: .ascii	"Error "		/* runs into newline... */
errcod: .byte	0			/* ... if errcod set */
newline:
	.asciz	"\r\n"

#ifndef TERSE_ERROR
ERR_READ:		.asciz	"Disk read"
ERR_NO_BOOT:		.asciz	"No /boot"
ERR_NOT_FAT16:		.asciz	"Not FAT16 ptn"
ERR_NO_BOOT_MAGIC_2:	.asciz	"No magic in /boot"
#endif

boot_filename: .ascii	"BOOT       "

space:
pbr_space = boot_params - .

/*
 * Add magic number, with a zero sized patchable area - just in case something
 * finds it and tries to update the area.
 * Boot options can be set using 'installboot -e boot' so we don't need to
 * use any of our valuable bytes.
 */

	. = _C_LABEL(start) + 0x1fe - 2 - 4 - 4
boot_params:
	.long	X86_BOOT_MAGIC_FAT
	.long	1f - .
1:
fat_cache:	.word	0xffff		/* Sector number in buffer */
	. = _C_LABEL(start) + 0x1fe
	.word	0xaa55
fat_buffer:				/* 2 sectors worth of FAT table */
