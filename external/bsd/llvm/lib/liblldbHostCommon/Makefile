#	$NetBSD: Makefile,v 1.2 2014/05/30 18:28:15 joerg Exp $

LIB=	lldbHostCommon

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Host/common

SRCS+=	Condition.cpp \
	DynamicLibrary.cpp \
	Editline.cpp \
	File.cpp \
	FileSpec.cpp \
	Host.cpp \
	Mutex.cpp \
	OptionParser.cpp \
	ProcessRunLock.cpp \
	SocketAddress.cpp \
	Symbols.cpp \
	Terminal.cpp \
	TimeValue.cpp

CPPFLAGS.OptionParser.cpp+=	-Dgetopt_long_only=getopt_long

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
