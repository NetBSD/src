#	$NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:39 tls Exp $

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
