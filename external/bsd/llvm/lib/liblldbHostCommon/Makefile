#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:48 yamt Exp $

LIB=	lldbHostCommon

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Host/common

SRCS+=	Condition.cpp \
	DynamicLibrary.cpp \
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
