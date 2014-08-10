#	$NetBSD: Makefile,v 1.1.2.1 2014/08/10 07:09:11 tls Exp $

LIB=	lldbUtility

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Utility

SRCS+=	ARM_DWARF_Registers.cpp \
	ARM64_DWARF_Registers.cpp \
	KQueue.cpp \
	PseudoTerminal.cpp \
	Range.cpp \
	SharingPtr.cpp \
	StringExtractor.cpp \
	StringExtractorGDBRemote.cpp \
	TimeSpecTimeout.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
