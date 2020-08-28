# $NetBSD: cmd-interrupt.mk,v 1.1 2020/08/28 15:40:53 rillig Exp $
#
# Tests for interrupting a command.
#
# If a command is interrupted (usually by the user, here by itself), the
# target is removed.  This is to avoid having an unfinished target that
# would be newer than all of its sources and would therefore not be
# tried again in the next run.
#
# For .PHONY targets, there is usually no corresponding file and therefore
# nothing that could be removed.  These .PHONY targets need to ensure for
# themselves that interrupting them does not leave an inconsistent state
# behind.

SELF:=	${.PARSEDIR}/${.PARSEFILE}

_!=	rm -f cmd-interrupt-ordinary cmd-interrupt-phony

all: interrupt-ordinary interrupt-phony

interrupt-ordinary:
	${.MAKE} ${MAKEFLAGS} -f ${SELF} cmd-interrupt-ordinary || true
	@echo ${exists(cmd-interrupt-ordinary) :? error : ok }

interrupt-phony:
	${.MAKE} ${MAKEFLAGS} -f ${SELF} cmd-interrupt-phony || true
	@echo ${exists(cmd-interrupt-phony) :? ok : error }

cmd-interrupt-ordinary:
	> ${.TARGET}
	kill -INT $$$$

cmd-interrupt-phony: .PHONY
	> ${.TARGET}
	kill -INT $$$$
