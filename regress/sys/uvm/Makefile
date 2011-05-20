#	$NetBSD: Makefile,v 1.8 2011/05/20 13:36:12 joerg Exp $

.include <bsd.own.mk>

SUBDIR+= mmap loan1

.if !empty(SUPPORTED_CC:Mgcc)
SUBDIR+= stack_exec
.endif

#
# Test for no-executable stack; applies only to architectures
# where CPU and kernel support it.
#
STACKNOX_ARCHS= alpha i386 powerpc sparc sparc64 x86_64

.if !empty(SUPPORTED_CC:Mgcc) && !empty(STACKNOX_ARCHS:M${MACHINE_ARCH})
SUBDIR+= stack_noexec
.endif

.include <bsd.subdir.mk>
