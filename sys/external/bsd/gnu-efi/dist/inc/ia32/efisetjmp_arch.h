/*	$NetBSD: efisetjmp_arch.h,v 1.2.6.2 2019/06/10 22:08:35 christos Exp $	*/

#ifndef GNU_EFI_IA32_SETJMP_H
#define GNU_EFI_IA32_SETJMP_H

#define JMPBUF_ALIGN 4

typedef struct {
	UINT32	Ebx;
	UINT32	Esi;
	UINT32	Edi;
	UINT32	Ebp;
	UINT32	Esp;
	UINT32	Eip;
} EFI_ALIGN(JMPBUF_ALIGN) jmp_buf;

#endif /* GNU_EFI_IA32_SETJMP_H */
