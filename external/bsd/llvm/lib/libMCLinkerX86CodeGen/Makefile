#	$NetBSD: Makefile,v 1.1.6.2 2013/06/23 06:28:25 tls Exp $

LIB=	MCLinkerX86CodeGen

.include <bsd.init.mk>

.PATH: ${MCLINKER_SRCDIR}/lib/Target/X86

SRCS=	X86Diagnostic.cpp \
	X86ELFDynamic.cpp \
	X86ELFMCLinker.cpp \
	X86Emulation.cpp \
	X86GOT.cpp \
	X86GOTPLT.cpp \
	X86LDBackend.cpp \
	X86MCLinker.cpp \
	X86PLT.cpp \
	X86Relocator.cpp \
	X86TargetMachine.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
