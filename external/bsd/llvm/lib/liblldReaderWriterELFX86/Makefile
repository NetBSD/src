#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:47 yamt Exp $

LIB=	lldReaderWriterELFX86

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/ReaderWriter/ELF/X86

CPPFLAGS+=	-I${LLD_SRCDIR}/lib/ReaderWriter/ELF

SRCS+=	X86LinkingContext.cpp \
	X86TargetHandler.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
