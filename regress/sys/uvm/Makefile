#	$NetBSD: Makefile,v 1.6.12.1 2008/09/24 16:35:51 wrstuden Exp $

SUBDIR+= mmap loan1

.if !defined(HAVE_PCC)
SUBDIR+= stack_exec
.endif

#
# Test for no-executable stack; applies only to architectures
# where CPU and kernel support it.
#
STACKNOX_ARCHS= alpha i386 powerpc sparc sparc64 x86_64

.if !defined(HAVE_PCC) && !empty(STACKNOX_ARCHS:M${MACHINE_ARCH})
SUBDIR+= stack_noexec
.endif

.include <bsd.subdir.mk>
