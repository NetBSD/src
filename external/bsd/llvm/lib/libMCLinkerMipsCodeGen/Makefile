#	$NetBSD: Makefile,v 1.1.6.2 2013/06/23 06:28:24 tls Exp $

LIB=	MCLinkerMipsCodeGen

.include <bsd.init.mk>

.PATH: ${MCLINKER_SRCDIR}/lib/Target/Mips

SRCS=	MipsDiagnostic.cpp \
	MipsELFDynamic.cpp \
	MipsELFMCLinker.cpp \
	MipsEmulation.cpp \
	MipsGOT.cpp \
	MipsLDBackend.cpp \
	MipsMCLinker.cpp \
	MipsRelocator.cpp \
	MipsTargetMachine.cpp


.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
