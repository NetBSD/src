/*	$NetBSD: start_rom.S,v 1.9 2003/02/01 14:48:19 dsl Exp $	*/
	
/*
 * ROM startup
 * mostly from freebsd:sys/arch/i396/boot/netboot/start2.S
 */

#include <machine/asm.h>
#define	addr32	.byte 0x67

reloc = RELOC

#ifndef BOOTSTACK
#define BOOTSTACK 0xfffc
#endif
bootstack = BOOTSTACK

	.text
	.code16
textstart:

/* At entry, the processor is in 16 bit real mode and the code is being
 * executed from an address it was not linked to. Code must be pic and
 * 32 bit sensitive until things are fixed up.
 */
	.word	0xaa55			/* bios extension signature */
	.byte	0			/* no. of 512B blocks - set by genprom */
	jmp	1f			/* enter from bios here */
	.byte	0			/* checksum */
#ifdef PCIROM
	. = 0x18			/* required offset of pointer */
	.word	(pcidata - textstart)
pcidata:
	.ascii	"PCIR"
	.word	PCI_VID
	.word	PCI_DID
	.word	0			/* pointer to vital product data */
	.word	(pcidataend - pcidata)
	.long	(0 + (PCI_CLASS << 8))	/* class, revision */
	.word	0			/* no. of 512B blocks - set by genprom */
	.word	0			/* revision */
	.byte	0			/* code type */
	.byte	0x80			/* "indicator" - last image */
	.word	0			/* reserved */
pcidataend:
#endif
1:
	pushl	%eax
	push	%ds
	xor	%ax,%ax
	mov	%ax, %ds

	/* check signature */
	mov	0x304, %ax
	cmp	$0x4d52, %ax		/* $0x4d52  == 'MR' */
	jz	2f	/* we have been here - don't clobber saved vector */

	/* save old INT19 vector to a secret location (???) */
	movw	0x64, %ax
	movw	%ax, 0x300
	movw	0x66, %ax
	movw	%ax, 0x302

	/* set INT19 vector to our entry */
	movl	$(_C_LABEL(start)-reloc), %eax
	movw	%ax, 0x64
	mov	%cs, %ax
	movw	%ax, 0x66

	/* set a signature (is this a safe location?) */
	movw	$0x4d52, %ax		/* 0x4d52 == 'MR' */
	movw	%ax, 0x304

2:	pop	%ds
	popl	%eax

#ifdef ROMDEBUG
	push	%ds
	pushl	%eax
	mov	%cs, %ax
	mov	%ax, %ds
	pop	%ax
	pushl	%esi
	movw	$(imesg-textstart), %si
	call	message
	popl	%esi
	pop	%ds
#endif

	lret

#ifdef ROMDEBUG

imesg:	.asciz "bsd rom installed\r\n"
cmesg:	.asciz "bsd rom called\r\n"
rmesg:	.asciz "bsd rom relocated\r\n"
emesg:	.asciz "bsd rom done\r\n"

/*
 * message: write the message in %ds:%esi to console
 */
message:
	pushl	%eax
	pushl	%ebx
	pushl	%edx

nextb:
	cld
	lodsb			/* load a byte into %al */
	testb	%al, %al
	jz	done

	movb	$0x0e, %ah
	movw	$0x0001, %bx
	int	$0x10

	jmp	nextb
done:

	movb	$0x00, %ah	/* wait for keypress */
	int	$0x16

	popl	%edx
	popl	%ebx
	popl	%eax
	ret

#endif /* ROMDEBUG */

/**************************************************************************
START - Where all the fun begins....
**************************************************************************/

ENTRY(start)
	.code16
	cli
	mov	%cs, %ax
	mov	%ax, %ds

#ifdef ROMDEBUG
	movw	$(cmesg-textstart), %si
	call	message
#endif

	cld

	/* copy to reloc and jump to copy */
        xor	%si, %si
	xor	%di, %di
	movw	$(reloc>>4), %ax
        mov	%ax, %es
	movl	$(_C_LABEL(edata)-reloc), %ecx
        cs
        rep
        movsb
	ljmpl	$(reloc>>4), $1f-reloc		/* Jmp to RELOC:1f */
1:
	nop
	mov	%cs, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss
	movw	$bootstack, %ax
	mov	%ax, %sp

	/* clear bss */
	xor	%ax, %ax
	movl	$_C_LABEL(edata), %edi
	movl	$_C_LABEL(end), %ecx
	subw	%di, %cx
	cld
	rep
	stosb

#ifdef ROMDEBUG
	mov	$(rmesg-textstart), %si
	call	message
#endif

	calll	_C_LABEL(real_to_prot)
	.code32
	call	_C_LABEL(main)

	.globl	_C_LABEL(exit)
_C_LABEL(exit):
	call	_C_LABEL(prot_to_real)
	.code16

#ifdef ROMDEBUG
	mov	$(emesg-textstart), %si
	call	message
#endif

	/* jump to saved vector */
	xor	%ax, %ax
	mov	%ax, %ds
	movw	0x302, %ax
	push	%ax
	movw	0x300, %ax
	push	%ax
	lret

/**************************************************************************
GLOBAL DESCRIPTOR TABLE
**************************************************************************/
#ifdef __ELF__
	.align	16
#else
	.align	4
#endif
gdt:
	.word	0, 0
	.byte	0, 0x00, 0x00, 0

#ifdef SUPPORT_LINUX	/* additional dummy */
	.word	0, 0
	.byte	0, 0x00, 0x00, 0
#endif

	/* code segment */
	.globl flatcodeseg
flatcodeseg = . - gdt
bootcodeseg = . - gdt
	.word	0xffff, 0
	.byte	0, 0x9f, 0xcf, 0

	/* data segment */
	.globl flatdataseg
flatdataseg = . - gdt
bootdataseg = . - gdt
	.word	0xffff, 0
	.byte	0, 0x93, 0xcf, 0

	/* 16 bit real mode */
bootrealseg = . - gdt
	.word	0xffff, 0
	.byte	0, 0x9f, 0x0f, 0
gdtlen = . - gdt

#ifdef __ELF__
	.align	16
#else
	.align	4
#endif
gdtarg:
	.word	gdtlen-1		/* limit */
	.long	gdt			/* addr */

CR0_PE		=	0x1

/**************************************************************************
REAL_TO_PROT - Go from REAL mode to Protected Mode
**************************************************************************/
ENTRY(real_to_prot)
	.code16
	cli
	pushl	%eax

	.code32
	addr32				/* don't know the syntax for this! */
	lgdt	%cs:gdtarg-reloc
	.code16

	movl	%cr0, %eax
	orl	$CR0_PE, %eax
	mov	%eax, %cr0		/* turn on protected mode */

	/* jump to relocation, flush prefetch queue, and reload %cs */
	ljmpl	$bootcodeseg, $1f
1:
	.code32
	/* reload other segment registers */
	movl	$bootdataseg, %eax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss
	add	$reloc, %esp		/* Fix up stack pointer */
	movl	4(%esp), %eax		/* Fix up return Address */
	add	$reloc, %eax
	movl	%eax, 4(%esp)
	popl	%eax
	ret


/**************************************************************************
PROT_TO_REAL - Go from Protected Mode to REAL Mode
**************************************************************************/
ENTRY(prot_to_real)
	.code32
	pushl	%eax
	movl	4(%esp), %eax		/* Fix up return Address */
	sub	$reloc, %eax
	movl	%eax, 4(%esp)
	sub	$reloc, %esp		/* Adjust stack pointer */
	ljmp	$bootrealseg, $1f	/* jump to a 16 bit segment */
1:
	.code16
	/* clear the PE bit of CR0 */
	mov	%cr0, %eax
	andl 	$~CR0_PE, %eax
	mov	%eax, %cr0

	/* make intersegment jmp to flush the processor pipeline
	 * and reload CS register
	 */
	ljmpl	$(reloc)>>4, $2f-reloc
2:
	/* we are in real mode now
	 * set up the real mode segment registers : DS, SS, ES
	 */
	mov	%cs, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss
	sti
	popl	%eax
	retl

ENTRY(pbzero)
	.code32
	jmp	_C_LABEL(bzero)

ENTRY(vpbcopy)
ENTRY(pvbcopy)
	.code32
	jmp	_C_LABEL(bcopy)

ENTRY(vtophys)
	.code32
	movl	4(%esp), %eax
	ret
