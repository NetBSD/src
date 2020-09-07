#	$NetBSD: Makefile,v 1.2 2020/09/07 00:18:25 mrg Exp $

LIB=	LLVMTransformsCFGuard

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/CFGuard

SRCS+=	CFGuard.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif

CWARNFLAGS.gcc+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 9:? -Wno-error=init-list-lifetime :}
