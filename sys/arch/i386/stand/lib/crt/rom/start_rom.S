/*	$NetBSD: start_rom.S,v 1.8.2.2 1999/11/07 01:06:38 mycroft Exp $	*/
	
/*
 * ROM startup
 * mostly from freebsd:sys/arch/i396/boot/netboot/start2.S
 */

#include <machine/asm.h>
#define	addr32	.byte 0x67
#define	data32	.byte 0x66

reloc = RELOC

#ifndef BOOTSTACK
#define BOOTSTACK 0xfffc
#endif
bootstack = BOOTSTACK

	.text
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
	data32
	pushl	%eax
	push	%ds
	xorl	%eax,%eax
	movl	%ax, %ds

	/* check signature */
	.byte	0xa1			/* MOV 0x304,%ax */
	.word	0x304
	.byte	0x3d			/* CMP $0x4d52, %ax  == 'MR' */
	.word	0x4d52
	jz	2f	/* we have been here - don't clobber saved vector */

	/* save old INT19 vector to a secret location (???) */
	.byte	0xa1			/* MOV 0x64, %ax */
	.word	0x64
	.byte	0xa3			/* MOV %ax, 0x300 */
	.word	0x300
	.byte	0xa1			/* MOV 0x66, %ax */
	.word	0x66
	.byte	0xa3			/* MOV %ax, 0x302 */
	.word	0x302

	/* set INT19 vector to our entry */
	data32
	movl	$(_C_LABEL(start)-reloc), %eax
	.byte	0xa3			/* MOV %ax, 0x64 */
	.word	0x64
	movl	%cs, %ax
	.byte	0xa3			/* MOV %ax, 0x66 */
	.word	0x66

	/* set a signature (is this a save location?) */
	.byte	0xb8			/* MOV 'MR',%ax */
	.word	0x4d52
	.byte	0xa3			/* MOV %ax, 0x304 */
	.word	0x304

2:	pop	%ds
	data32
	popl	%eax

#ifdef ROMDEBUG
	push	%ds
	pushl	%eax
	movl	%cs, %ax
	movl	%ax, %ds
	popl	%eax
	data32
	pushl	%esi
	data32
	movl $(imesg-textstart), %esi
	data32
	call message
	data32
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
	data32
	pushl	%eax
	data32
	pushl	%ebx
	data32
	pushl	%edx

nextb:
	cld
	lodsb			/* load a byte into %al */
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

	movb	$0x00, %ah	/* wait for keypress */
	int $0x16

	data32
	popl	%edx
	data32
	popl	%ebx
	data32
	popl	%eax
	data32
	ret

#endif /* ROMDEBUG */

/**************************************************************************
START - Where all the fun begins....
**************************************************************************/

ENTRY(start)
	cli
	movl	%cs, %ax
	movl	%ax, %ds

#ifdef ROMDEBUG
	data32
	movl $(cmesg-textstart), %esi
	data32
	call message
#endif

	cld

	/* copy to reloc and jump to copy */
        xorl	%esi, %esi
	xorl	%edi, %edi
	data32
	movl	$(reloc>>4), %eax
        movl	%ax, %es
	data32
	movl	$(_C_LABEL(edata)-reloc), %ecx
        cs
        rep
        movsb
	data32
	ljmp	$(reloc>>4), $1f-reloc		/* Jmp to RELOC:1f */
1:
	nop
	movl	%cs, %ax
	movl	%ax, %ds
	movl	%ax, %es
	movl	%ax, %ss
#if 0 /* XXX why doesn't this work? */
	data32
	movl	$bootstack, %eax
	movl	%eax, %esp
#else
	.byte	0xb8			/* MOV $STACKADDR, %ax */
	.word	bootstack
	movl	%eax, %esp
#endif

	/* clear bss */
	xorl %eax, %eax
	data32
	movl $_C_LABEL(edata), %edi
	data32
	movl $_C_LABEL(end), %ecx
	subl %edi, %ecx
	cld
	rep
	stosb

#ifdef ROMDEBUG
	data32
	movl $(rmesg-textstart), %esi
	data32
	call message
#endif

	data32
	call	_C_LABEL(real_to_prot)
	call	_C_LABEL(main)

	.globl	_C_LABEL(exit)
_C_LABEL(exit):
	call	_C_LABEL(prot_to_real)

#ifdef ROMDEBUG
	data32
	movl $(emesg-textstart), %esi
	data32
	call message
#endif

	/* jump to saved vector */
	xor	%eax, %eax
	movl	%ax, %ds
	.byte	0xa1			/* MOV 0x302, %ax */
	.word	0x302
	push	%eax
	.byte	0xa1			/* MOV 0x300, %ax */
	.word	0x300
	push	%eax
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
	cli

	cs
	addr32
	lgdt	gdtarg-reloc

	movl	%cr0, %eax
	data32
	orl	$CR0_PE, %eax
	mov	%eax, %cr0		/* turn on protected mode */

	/* jump to relocation, flush prefetch queue, and reload %cs */
	data32
	ljmp	$bootcodeseg, $1f
1:
	/* reload other segment registers */
	movl	$bootdataseg, %eax
	movl	%ax, %ds
	movl	%ax, %es
	movl	%ax, %ss
	add	$reloc, %esp		/* Fix up stack pointer */
	pop	%eax			/* Fix up return Address */
	add	$reloc, %eax
	push	%eax
	ret


/**************************************************************************
PROT_TO_REAL - Go from Protected Mode to REAL Mode
**************************************************************************/
ENTRY(prot_to_real)
	pop	%eax
	sub	$reloc, %eax		/* Adjust return address */
	push	%eax
	sub	$reloc, %esp		/* Adjust stack pointer */
	ljmp	$bootrealseg, $1f	/* jump to a 16 bit segment */
1:
	/* clear the PE bit of CR0 */
	mov	%cr0, %eax
	data32
	andl 	$~CR0_PE, %eax
	mov	%eax, %cr0

	/* make intersegment jmp to flush the processor pipeline
	 * and reload CS register
	 */
	data32
	ljmp	$(reloc)>>4, $2f-reloc
2:
	/* we are in real mode now
	 * set up the real mode segment registers : DS, SS, ES
	 */
	movl	%cs, %ax
	movl	%ax, %ds
	movl	%ax, %es
	movl	%ax, %ss
	sti
	data32
	ret

ENTRY(pbzero)
	jmp	_C_LABEL(bzero)

ENTRY(vpbcopy)
ENTRY(pvbcopy)
	jmp	_C_LABEL(bcopy)

ENTRY(vtophys)
	movl 4(%esp), %eax
	ret
