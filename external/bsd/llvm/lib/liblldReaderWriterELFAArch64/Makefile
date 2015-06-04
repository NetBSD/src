#	$NetBSD: Makefile,v 1.1.2.2 2015/06/04 20:05:46 snj Exp $

LIB=	lldReaderWriterELFAArch64

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/ELF/AArch64

CPPFLAGS+=	-I${LLD_SRCDIR}/lib/ReaderWriter/ELF

SRCS+=	AArch64LinkingContext.cpp \
	AArch64RelocationHandler.cpp \
	AArch64RelocationPass.cpp \
	AArch64TargetHandler.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
