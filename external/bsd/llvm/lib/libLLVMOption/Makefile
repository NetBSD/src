#	$NetBSD: Makefile,v 1.1.12.2 2014/05/22 16:14:43 yamt Exp $

LIB=	LLVMOption

NOGCCERROR=	yes

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
