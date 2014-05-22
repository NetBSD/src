#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:50 yamt Exp $

LIB=	lldbUtility

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Utility

SRCS+=	ARM_DWARF_Registers.cpp \
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
