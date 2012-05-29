#	$NetBSD: Makefile,v 1.4 2012/05/29 19:31:52 joerg Exp $

LIB=	clangDriver

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Driver

SRCS+=	Action.cpp \
	Arg.cpp \
	ArgList.cpp \
	CC1AsOptions.cpp \
	Compilation.cpp \
	Driver.cpp \
	DriverOptions.cpp \
	Job.cpp \
	Option.cpp \
	OptTable.cpp \
	Phases.cpp \
	Tool.cpp \
	ToolChain.cpp \
	ToolChains.cpp \
	Tools.cpp \
	Types.cpp \
	WindowsToolChain.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
