| file: boot_ustar.S
| author: chapuni(webmaster@chapuni.com)
|         Yasha(itohy@netbsd.org)
|	  minoura@netbsd.org
|
| $NetBSD: boot_ustar.S,v 1.2 2001/10/01 16:58:01 minoura Exp $

| supports floppy only

#include <sys/reboot.h>
#include <machine/asm.h>
#include <machine/bootinfo.h>
#include "iocscall.h"

#define BOOT_ERROR(s)	jbsr boot_error; .asciz s; .even
#define SRAM_MEMSIZE	(0x00ed0008)

	.text
ASENTRY_NOPROFILE(start)
ASENTRY_NOPROFILE(top)
		bras	_ASM_LABEL(entry0)
		.ascii	"SHARP/"
		.ascii	"X680x0"
		.word	0x8199,0x94e6,0x82ea,0x82bd
		.word	0x8e9e,0x82c9,0x82cd,0x8cbb
		.word	0x8ec0,0x93a6,0x94f0,0x8149
		.word	0
ASENTRY_NOPROFILE(entry0)
		IOCS(__BOOTINF)
		lsll	#8,%d0		| clear MSByte
		lsrl	#8,%d0		|

		|
		| 0x80...0x8F		SASI
		| 0x90...0x93		Floppy
		| 0xED0000...0xED3FFE	SRAM
		| others		ROM (SCSI?)
		|
		movel	%d0,%d1
		clrb	%d1
		tstl	%d1
		jne	boot_dev_unsupported

		bra	_ASM_LABEL(entry)

|	Disklabel= 404bytes
|	Since LABELLOFFSET in <machine/disklabel.h> is 0x40,
|	entry must be after 0x000001d4 (0x000021d4)
		nop
GLOBAL(disklabel)
		.space	404

ASENTRY_NOPROFILE(entry)
		|
		| SASI or Floppy
		|
		movel	%d0,%d6
		andib	#0xFC,%d0
		cmpib	#0x90,%d0
		jne	boot_dev_unsupported	| boot from SASI?
		|
		| Floppy
		|   read first 64KB (XXX: CONSTANT!!)
		|
		andib	#0x03,%d0	| drive # (head=0)
		jbsr	check_fd_format
		moveb	%d6,%d2
		lslw	#8,%d2
		moveq	#0x70,%d1
		orw	%d2,%d1		| PDA*256 + MODE
		movel	%d0,%d2		| read position (first sector)
		movel	#65536,%d3	| read bytes
		moval	#(BOOT_TEXTADDR-8192-0x200-32),%a1
		moval	%a1,%a4		| save buffer addr
		IOCS(__B_READ)
		jra	boot_read_done

#include "chkfmt.s"

boot_dev_unsupported:
		BOOT_ERROR("unsupported boot device")

booterr_msg:	.ascii	"\r\n\n"
		.ascii	BOOT
		.asciz	": "
reboot_msg:	.asciz	"\r\n[Hit key to reboot]"
		.even

ASENTRY_NOPROFILE(boot_error)
		lea	%pc@(booterr_msg),%a1
		IOCS(__B_PRINT)
		moveal	%sp@+,%a1
		IOCS(__B_PRINT)
		lea	%pc@(reboot_msg),%a1
		IOCS(__B_PRINT)

		| wait for a key press (or release of a modifier)
		IOCS(__B_KEYINP)

		| issue software reset
		trap	#10
		| NOTREACHED

boot_read_done:
		lea	%a4@(8192),%a1		| USTAR header
		cmpl	#0x55535441,%a1@	| filename `USTA
		bne	error_invalidname
		cmpl	#0x522e766f,%a1@(4)	|           R.vo...'
		bne	error_invalidname
		cmpl	#0x00757374,%a1@(256)	| magic `\0ust'
		bne	error_invalidfs
		cmpl	#0x61722020,%a1@(260)	| magic `ar  '
		bne	error_invalidfs

		lea	%a1@(0x200),%a2		| a.out header
		movml	%a2@+,%d0-%d4/%a5
		addal	#8,%a2			| start of text

		cmpal	#BOOT_TEXTADDR,%a5
		bne	error_invalidboot

		cmpl	#0x00870107,%d0		| a.out magic
		bne	error_invalidmagic

		addl	%d2,%d1			| a_text+a_data
		movl	%d3,%d4			| save a_bss
		addl	%d1,%d3			| a_text+a_data+a_bss

	/* clear out bss */
		lea	%a2@(%d1),%a3
1:		movb	#0,%a3@+
		subl	#1,%d4
		bne	1b

	/* set args for /boot */
		pea	%a2@(%d3)		| esym (unused)
		movel	SRAM_MEMSIZE,%sp@-	| lastpa (unused)
		pea	%a2@			| firstpa (unused)

		movq	#0,%d0			| unused
		movq	#0,%d1			| unused
		movq	#0,%d2			| unused
		movq	#0,%d3			| unused
		movq	#0,%d4			| unused
		movq	#0,%d5			| unused
		andil	#3,%d6			| bootdev
		swap	%d6
		addil	#(B_DEVMAGIC+X68K_MAJOR_FD),%d6
		movl	#RB_SINGLE,%d7		| boothowto (unused)
		moval	%a2,%a0			| entry
		moval	%d0,%a1			| unused
		moval	%d0,%a2			| unused
		moval	%d0,%a3			| unused
		moval	%d0,%a4			| unused
		moval	%d0,%a5			| unused
		moval	%d0,%a6			| unused

		jmp	%a0@			| here we go

error_invalidboot:	BOOT_ERROR("Illegal secondary boot");
error_invalidname:	BOOT_ERROR("Invalid filename");
error_invalidfs:	BOOT_ERROR("Not a ustarfs");
error_invalidmagic:	BOOT_ERROR("Not an OMAGIC a.out")

ASGLOBAL(first_kbyte)
