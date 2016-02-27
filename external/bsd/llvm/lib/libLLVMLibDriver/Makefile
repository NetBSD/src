#	$NetBSD: Makefile,v 1.1 2016/02/27 21:47:33 joerg Exp $

LIB=	LLVMLibDriver

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/LibDriver

SRCS+=	LibDriver.cpp

TABLEGEN_SRC=		Options.td
TABLEGEN_OUTPUT= \
	Options.inc|-gen-opt-parser-defs

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
