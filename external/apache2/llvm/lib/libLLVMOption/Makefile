#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:10 joerg Exp $

LIB=	LLVMOption

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Option

SRCS+=	ArgList.cpp  \
	Arg.cpp \
	OptTable.cpp \
	Option.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
