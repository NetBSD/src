/*	$NetBSD: comio.S,v 1.1.1.1 1997/03/14 02:40:32 perry Exp $	*/

/* serial console handling
  modelled after code in FreeBSD:sys/i386/boot/netboot/start2.S
 */

#include <machine/asm.h>

#define	addr32	.byte 0x67
#define	data32	.byte 0x66

	.text

/**************************************************************************
INIT - Initialization (com number)
**************************************************************************/
ENTRY(cominit)
	push	%ebp
	mov	%esp,%ebp
	push	%ebx
	push	%edx
	push	%esi
	push	%edi

	movl	8(%ebp), %edx

	call	_C_LABEL(prot_to_real)	# enter real mode

	# Initialize the serial port (dl) to 9600 baud, 8N1.
	movb	$0xe3, %al
	movb	$0, %ah
	int	$0x14
	mov	%ax,%bx

	data32
	call	_C_LABEL(real_to_prot) # back to protected mode

	xor	%eax,%eax
	mov	%bx,%ax

	pop	%edi
	pop	%esi
	pop	%edx
	pop	%ebx
	pop	%ebp
	ret

/**************************************************************************
PUTC - Print a character (char, com number)
**************************************************************************/
ENTRY(computc)
	push	%ebp
	mov	%esp,%ebp
	push	%ecx
	push	%ebx
	push	%edx
	push	%esi
	push	%edi

	movb	8(%ebp),%cl
	movl	12(%ebp),%edx

	call	_C_LABEL(prot_to_real)	# enter real mode

	movb	%cl,%al
	movb	$0x01, %ah
	int	$0x14

	movb	%ah,%bl

	data32
	call	_C_LABEL(real_to_prot) # back to protected mode

	xor	%eax,%eax
	movb	%bl,%al

	pop	%edi
	pop	%esi
	pop	%edx
	pop	%ebx
	pop	%ecx
	pop	%ebp
	ret

/**************************************************************************
GETC - Get a character (com number)
**************************************************************************/
ENTRY(comgetc)
	push	%ebp
	mov	%esp,%ebp
	push	%ebx
	push	%edx
	push	%esi
	push	%edi

	movl	8(%ebp),%edx

	call	_C_LABEL(prot_to_real)	# enter real mode

	movb	$0x02, %ah
	int	$0x14
	movb	%al,%bl

	data32
	call	_C_LABEL(real_to_prot) # back to protected mode

	xor	%eax,%eax
	movb	%bl,%al

	pop	%edi
	pop	%esi
	pop	%edx
	pop	%ebx
	pop	%ebp
	ret

/**************************************************************************
ISKEY - Check for keyboard interrupt (com number)
**************************************************************************/
ENTRY(comstatus)
	push	%ebp
	mov	%esp,%ebp
	push	%ebx
	push	%edx
	push	%esi
	push	%edi

	movl	8(%ebp),%edx

	call	_C_LABEL(prot_to_real)	# enter real mode

	movb	$0x03, %ah
	int	$0x14
	mov	%ax,%bx

	data32
	call	_C_LABEL(real_to_prot) # back to protected mode

	xor	%eax,%eax
	mov	%bx,%ax

	pop	%edi
	pop	%esi
	pop	%edx
	pop	%ebx
	pop	%ebp
	ret
