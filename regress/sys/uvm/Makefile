#	$NetBSD: Makefile,v 1.5.16.1 2007/07/19 14:38:23 liamjfoy Exp $

SUBDIR+= mmap stack_exec loan1

#
# Test for no-executable stack; applies only to architectures
# where CPU and kernel support it.
#
STACKNOX_ARCHS= alpha i386 powerpc sparc sparc64 x86_64

.if !empty(STACKNOX_ARCHS:M${MACHINE_ARCH})
SUBDIR+= stack_noexec
.endif

.include <bsd.subdir.mk>
