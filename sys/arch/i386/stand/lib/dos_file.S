/*	$NetBSD: dos_file.S,v 1.1.1.1 1997/03/14 02:40:32 perry Exp $	*/
	
/* extracted from Tor Egge's patches for NetBSD boot */

#include <machine/asm.h>

#define	addr32	.byte 0x67
#define	data32	.byte 0x66

/*
# MSDOS call "INT 0x21 Function 0x3d" to open a file.
# Call with	%ah = 0x3d
# 		%al = 0x0  (access and sharing modes)
#		%ds:%dx = ASCIZ filename
#		%cl = attribute mask of files to look for
*/

	.globl _doserrno
_doserrno:	.long 1

ENTRY(dosopen)
	pushl %ebp
	movl  %esp, %ebp
	pushl	%edx
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	
	movl	0x8(%ebp), %edx # File name.

	call	_C_LABEL(prot_to_real)	# enter real mode

	movb	$0x3d, %ah	# Open existing file.
	movb	$0x0 , %al	# ro

	sti
	int	$0x21
	cli

	jnc	ok1
	addr32
	movl	%eax, _C_LABEL(doserrno)
	data32
	movl	$-1, %edx
	jmp err1
ok1:
	data32
	movl	$0,%edx
	mov %ax, %dx
err1:
	data32
	call	_C_LABEL(real_to_prot) # back to protected mode

	movl	%edx, %eax		# return value in %eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%edx
	popl	%ebp
	ret

ENTRY(dosread)
	pushl %ebp
	movl  %esp, %ebp
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
	pushl	%edi
	
	movl	0x8(%ebp), %ebx # File handle
	movl	0xc(%ebp), %edx # Buffer.
	movl	0x10(%ebp) , %ecx	# Bytes to read

	call	_C_LABEL(prot_to_real)	# enter real mode

	movb	$0x3f, %ah	# Read from file or device

	sti
	int	$0x21
	cli

	jnc	ok2
	addr32
	movl	%eax, _C_LABEL(doserrno)
	data32
	movl	$-1, %edx
	jmp	err2
ok2:
	data32
	movl	$0,%edx
	mov	%ax, %dx
err2:
	data32
	call	_C_LABEL(real_to_prot) # back to protected mode

	movl	%edx, %eax		# return value in %eax

	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp
	ret

ENTRY(dosclose)
	pushl %ebp
	movl  %esp, %ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	
	movl	0x8(%ebp), %ebx # File handle

	call	_C_LABEL(prot_to_real)	# enter real mode

	movb	$0x3e, %ah	# Close file.

	sti
	int	$0x21
	cli

	jnc	ok3
	addr32
	movl	%eax, _C_LABEL(doserrno)
	data32
	movl	$-1, %ebx
	jmp	err3
ok3:
	data32
	movl	$0,%ebx
err3:
	data32
	call	_C_LABEL(real_to_prot) # back to protected mode

	movl	%ebx, %eax		# return value in %eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

ENTRY(dosseek)
	pushl %ebp
	movl  %esp, %ebp
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
	pushl	%edi
	
	movl	0x8(%ebp), %ebx # File handle
	movl	0xc(%ebp), %ecx # Offset
	movl	0x10(%ebp) , %edx	# whence

	call	_C_LABEL(prot_to_real)	# enter real mode

	movb	$0x42, %ah	# Seek
	movb	%dl , %al	# whence
	mov	%cx, %dx	#offs lo
	data32
	shr	$0x10, %ecx	#offs hi

	sti
	int	$0x21
	cli

	jnc	ok4
	addr32
	movl	%eax, _C_LABEL(doserrno)
	data32
	movl	$-1, %edx
	jmp	err4
ok4:
	data32
	shl	$0x10, %edx	#new ofs hi
	mov	%ax, %dx	#new ofs lo
err4:
	data32
	call	_C_LABEL(real_to_prot) # back to protected mode

	movl	%edx, %eax		# return value in %eax

	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp
	ret
