/*	$NetBSD: efisetjmp_arch.h,v 1.1.1.2 2021/09/30 18:50:09 jmcneill Exp $	*/

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
} ALIGN(JMPBUF_ALIGN) jmp_buf[1];

#endif /* GNU_EFI_IA32_SETJMP_H */
