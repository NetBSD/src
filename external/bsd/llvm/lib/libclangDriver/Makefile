#	$NetBSD: Makefile,v 1.8.2.1 2015/06/04 20:05:45 snj Exp $

LIB=	clangDriver

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Driver

SRCS+=	Action.cpp \
	Compilation.cpp \
	CrossWindowsToolChain.cpp \
	Driver.cpp \
	DriverOptions.cpp \
	Job.cpp \
	MSVCToolChain.cpp \
	Multilib.cpp \
	Phases.cpp \
	SanitizerArgs.cpp \
	Tool.cpp \
	ToolChain.cpp \
	ToolChains.cpp \
	Tools.cpp \
	Types.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
