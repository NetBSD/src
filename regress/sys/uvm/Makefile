#	$NetBSD: Makefile,v 1.4 2004/02/19 16:56:52 drochner Exp $

SUBDIR+= mmap stack_exec loan1

#
# Test for no-executable stack; applies only to architectures
# where CPU and kernel support it.
#
STACKNOX_ARCHS= alpha amd64 i386 powerpc sh5 sparc sparc64

.if !empty(STACKNOX_ARCHS:M${MACHINE_ARCH})
SUBDIR+= stack_noexec
.endif

.include <bsd.subdir.mk>
