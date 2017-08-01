#	$NetBSD: Makefile,v 1.1 2017/08/01 20:07:42 joerg Exp $

LIB=	LLVMToolDrivers

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ToolDrivers/llvm-lib

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
