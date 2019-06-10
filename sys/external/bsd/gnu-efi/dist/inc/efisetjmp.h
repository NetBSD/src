/*	$NetBSD: efisetjmp.h,v 1.1.1.1.6.2 2019/06/10 22:08:35 christos Exp $	*/

#ifndef GNU_EFI_SETJMP_H
#define GNU_EFI_SETJMP_H

#include "eficompiler.h"
#include "efisetjmp_arch.h"

extern UINTN setjmp(jmp_buf *env) __attribute__((returns_twice));
extern VOID longjmp(jmp_buf *env, UINTN value) __attribute__((noreturn));

#endif /* GNU_EFI_SETJMP_H */
