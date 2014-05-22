#	$NetBSD: Makefile,v 1.1.12.2 2014/05/22 16:14:45 yamt Exp $

LIB=	MCLinkerTarget

.include <bsd.init.mk>

.PATH: ${MCLINKER_SRCDIR}/lib/Target

SRCS=	ELFDynamic.cpp \
	ELFEmulation.cpp \
	ELFMCLinker.cpp \
	GNUInfo.cpp \
	GNULDBackend.cpp \
	GOT.cpp \
	OutputRelocSection.cpp \
	PLT.cpp \
	Target.cpp \
	TargetLDBackend.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
