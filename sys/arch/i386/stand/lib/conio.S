/*	$NetBSD: conio.S,v 1.6 2008/05/21 01:51:34 ad Exp $	*/

/* PC console handling
  originally from: FreeBSD:sys/i386/boot/netboot/start2.S
 */

#include <machine/asm.h>

	.text

/**************************************************************************
CLR - Clear screen
**************************************************************************/
ENTRY(conclr)
	push	%ebp
	mov	%esp,%ebp
	push	%ecx
	push	%ebx
	push	%esi
	push	%edi

	movb	8(%ebp),%cl

	call	_C_LABEL(prot_to_real)	# enter real mode
	.code16

	/* Clear screen. */
	movw	$0x0600, %ax
	movw	$0x0700, %bx
	xorw	%cx, %cx
	movw	$0x184f, %dx	/* 80x25 */
	int	$0x10

	/* Home cursor. */
	movb	$0x02, %ah
	xorw	%bx, %bx
	xorw	%dx, %dx
	int	$0x10

	calll	_C_LABEL(real_to_prot) # back to protected mode
	.code32

	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ecx
	pop	%ebp
	ret

/**************************************************************************
PUTC - Print a character
**************************************************************************/
ENTRY(conputc)
	push	%ebp
	mov	%esp,%ebp
	push	%ecx
	push	%ebx
	push	%esi
	push	%edi

	movb	8(%ebp),%cl

	call	_C_LABEL(prot_to_real)	# enter real mode
	.code16

	movb	%cl,%al
	movw	$1,%bx
	movb	$0x0e,%ah
	int	$0x10

	calll	_C_LABEL(real_to_prot) # back to protected mode
	.code32

	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ecx
	pop	%ebp
	ret

/**************************************************************************
GETC - Get a character
**************************************************************************/
ENTRY(congetc)
	push	%ebp
	mov	%esp,%ebp
	push	%ebx
	push	%esi
	push	%edi

	call	_C_LABEL(prot_to_real)	# enter real mode
	.code16

	movb	$0x0,%ah
	int	$0x16
	movb	%al,%bl

	calll	_C_LABEL(real_to_prot) # back to protected mode
	.code32

	xor	%eax,%eax
	movb	%bl,%al

	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ebp
	ret

/**************************************************************************
ISSHIFT - Check for keyboard interrupt; via shift key
**************************************************************************/
ENTRY(conisshift)
	push	%ebp
	mov	%esp,%ebp
	push	%ebx
	push	%esi
	push	%edi

	call	_C_LABEL(prot_to_real)	# enter real mode
	.code16

	xor	%bx,%bx
	movb	$0x2,%ah
	int	$0x16
	testb	$3,%al
	setnz	%bl

	calll	_C_LABEL(real_to_prot) # back to protected mode
	.code32

	xor	%eax,%eax
	movb	%bl,%al

	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ebp
	ret

/**************************************************************************
ISKEY - Check for keyboard input
**************************************************************************/
ENTRY(coniskey)
	push	%ebp
	mov	%esp,%ebp
	push	%ebx
	push	%esi
	push	%edi

	call	_C_LABEL(prot_to_real)	# enter real mode
	.code16

	xor	%bx,%bx
	movb	$0x1,%ah
	int	$0x16
	setnz	%bl

	calll	_C_LABEL(real_to_prot) # back to protected mode
	.code32

	xor	%eax,%eax
	movb	%bl,%al

	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ebp
	ret
