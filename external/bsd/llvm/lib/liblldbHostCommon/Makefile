#	$NetBSD: Makefile,v 1.3 2014/08/10 17:35:07 joerg Exp $

LIB=	lldbHostCommon

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Host/common

SRCS+=	Condition.cpp \
	DynamicLibrary.cpp \
	Editline.cpp \
	File.cpp \
	FileSpec.cpp \
	Host.cpp \
	IOObject.cpp \
	Mutex.cpp \
	NativeBreakpoint.cpp \
	NativeBreakpointList.cpp \
	NativeProcessProtocol.cpp \
	NativeThreadProtocol.cpp \
	OptionParser.cpp \
	Pipe.cpp \
	ProcessRunLock.cpp \
	Socket.cpp \
	SocketAddress.cpp \
	SoftwareBreakpoint.cpp \
	Symbols.cpp \
	Terminal.cpp \
	TimeValue.cpp

CPPFLAGS.OptionParser.cpp+=	-Dgetopt_long_only=getopt_long

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
