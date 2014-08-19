#	$NetBSD: Makefile,v 1.2.4.2 2014/08/19 23:51:27 tls Exp $

LIB=	lldbCommands

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Commands

CPPFLAGS.CommandObjectGUI.cpp+= -DLLDB_DISABLE_CURSES

SRCS+=	CommandCompletions.cpp \
	CommandObjectApropos.cpp \
	CommandObjectArgs.cpp \
	CommandObjectBreakpoint.cpp \
	CommandObjectBreakpointCommand.cpp \
	CommandObjectCommands.cpp \
	CommandObjectDisassemble.cpp \
	CommandObjectExpression.cpp \
	CommandObjectFrame.cpp \
	CommandObjectGUI.cpp \
	CommandObjectHelp.cpp \
	CommandObjectLog.cpp \
	CommandObjectMemory.cpp \
	CommandObjectMultiword.cpp \
	CommandObjectPlatform.cpp \
	CommandObjectPlugin.cpp \
	CommandObjectProcess.cpp \
	CommandObjectQuit.cpp \
	CommandObjectRegister.cpp \
	CommandObjectSettings.cpp \
	CommandObjectSource.cpp \
	CommandObjectSyntax.cpp \
	CommandObjectTarget.cpp \
	CommandObjectThread.cpp \
	CommandObjectType.cpp \
	CommandObjectVersion.cpp \
	CommandObjectWatchpoint.cpp \
	CommandObjectWatchpointCommand.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
