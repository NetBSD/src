/*	$NetBSD: start_rom.S,v 1.3 1999/01/22 22:43:45 thorpej Exp $	*/
	
/*
 * ROM startup
 * mostly from freebsd:sys/arch/i396/boot/netboot/start2.S
 */

#include <machine/asm.h>

#define CR0_PE 1

reloc = RELOC

#ifndef BOOTSTACK
#define BOOTSTACK 0xfffc
#endif
bootstack = BOOTSTACK

#define opsize		.byte 0x66
#define addrsize	.byte 0x67

	.text

/* At entry, the processor is in 16 bit real mode and the code is being
 * executed from an address it was not linked to. Code must be pic and
 * 32 bit sensitive until things are fixed up.
 */
#ifdef BOOTROM
	.word	0xaa55			/* bios extension signature */
	.byte	0			/* no. of 512B blocks - set by genprom */
	jmp	1f			/* enter from bios here */
	.byte	0			/* checksum */
#ifdef PCIROM
	. = 0x18			/* required offset of pointer */
	.word	(pcidata - reloc)
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
1:	push	%eax
	push	%ds
	xor	%eax,%eax
	mov	%ax,%ds
	.byte	0xa1			/* MOV 0x304,%ax */
	.word	0x304
	.byte	0x3d			/* CMP $0x4d52, %ax  == 'MR' */
	.word	0x4d52
	jz	2f
	.byte	0xa1			/* MOV 0x64, %ax */
	.word	0x64
	.byte	0xa3			/* MOV %ax, 0x300 */
	.word	0x300
	.byte	0xa1			/* MOV 0x66, %ax */
	.word	0x66
	.byte	0xa3			/* MOV %ax, 0x302 */
	.word	0x302
	.byte	0xb8			/* MOV $_C_LABEL(start)-RELOCADDR, %ax */
	.word	(_C_LABEL(start)-reloc)
	.byte	0xa3			/* MOV %ax, 0x64 */
	.word	0x64
	mov	%cs,%ax
	.byte	0xa3			/* MOV %ax, 0x66 */
	.word	0x66
	.byte	0xb8			/* MOV 'MR',%ax */
	.word	0x4d52
	.byte	0xa3			/* MOV %ax, 0x304 */
	.word	0x304
2:	pop	%ds
	pop	%eax
	lret
#endif

/**************************************************************************
START - Where all the fun begins....
**************************************************************************/

ENTRY(start)
	cli
	cld

#ifdef BOOTROM				/* relocate ourselves */
        xor     %esi, %esi		/* zero for ROMs */
#else
	.byte	0xbe			/* MOV $0x100,%si -- 100h for .COM */
	.word	0x100
#endif
	xor	%edi,%edi
	.byte	0xb8			/* MOV $RELOCADDR>>4, %ax */
	.word	(reloc>>4)
        mov     %ax, %es
	.byte	0xb9			/* MOV $ROMSIZE, %cx */
	.word   (_C_LABEL(edata)-reloc)
        cs
        rep
        movsb
	opsize
	ljmp	$(reloc>>4),$1f-reloc		/* Jmp to RELOC:1f */
1:
	nop
	mov	%cs,%ax
	mov	%ax,%ds
	mov	%ax,%es
	mov	%ax,%ss
	.byte	0xb8			/* MOV $STACKADDR, %ax */
	.word	bootstack
	mov	%eax,%esp

	/* clear bss */
	xorl %eax,%eax
	movl $_C_LABEL(edata),%edi
	movl $_C_LABEL(end),%ecx
	subl %edi,%ecx
	cld
	rep
	stosb

	opsize
	call	_C_LABEL(real_to_prot)
	call	_C_LABEL(main)

	.globl	_C_LABEL(exit)
_C_LABEL(exit):
	call	_C_LABEL(prot_to_real)
#ifdef BOOTROM
	xor	%eax,%eax
	mov	%ax,%ds
	.byte	0xa1			/* MOV 0x302, %ax */
	.word	0x302
	push	%eax
	.byte	0xa1			/* MOV 0x300, %ax */
	.word	0x300
	push	%eax
	lret
#else
	sti
	movb $0x4c,%ah /* return */
	int $0x21
#endif

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
	.byte	0, 0x9b, 0x0f, 0
gdtlen = . - gdt

	.align	4
gdtarg:
	.word	gdtlen-1		/* limit */
	.long	gdt			/* addr */

/**************************************************************************
REAL_TO_PROT - Go from REAL mode to Protected Mode
**************************************************************************/
ENTRY(real_to_prot)
	cli
	cs
	addrsize
	lgdt	gdtarg-reloc
	mov	%cr0, %eax
	opsize
	or	$CR0_PE, %eax
	mov	%eax, %cr0		/* turn on protected mode */

	/* jump to relocation, flush prefetch queue, and reload %cs */
	opsize
	ljmp	$bootcodeseg, $1f
1:
	/* reload other segment registers */
	movl	$bootdataseg, %eax
	movl	%ax, %ds
	movl	%ax, %es
	movl	%ax, %ss
	add	$reloc,%esp		/* Fix up stack pointer */
	pop	%eax			/* Fix up return Address */
	add	$reloc,%eax
	push	%eax
	ret


/**************************************************************************
PROT_TO_REAL - Go from Protected Mode to REAL Mode
**************************************************************************/
ENTRY(prot_to_real)
	pop	%eax
	sub	$reloc,%eax		/* Adjust return address */
	push	%eax
	sub	$reloc,%esp		/* Adjust stack pointer */
	ljmp	$bootrealseg, $1f	/* jump to a 16 bit segment */
1:
	/* clear the PE bit of CR0 */
	mov	%cr0, %eax
	opsize
	andl 	$0!CR0_PE, %eax
	mov	%eax, %cr0

	/* make intersegment jmp to flush the processor pipeline
	 * and reload CS register
	 */
	opsize
	ljmp	$(reloc)>>4, $2f-reloc
2:
	/* we are in real mode now
	 * set up the real mode segment registers : DS, SS, ES
	 */
	mov	%cs, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss
	sti
	opsize
	ret

ENTRY(pbzero)
	jmp	_C_LABEL(bzero)

ENTRY(vpbcopy)
ENTRY(pvbcopy)
	jmp	_C_LABEL(bcopy)

ENTRY(vtophys)
	movl 4(%esp), %eax
	ret
