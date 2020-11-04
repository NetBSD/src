# $NetBSD: cmdline-undefined.mk,v 1.1 2020/11/04 04:24:57 rillig Exp $
#
# Tests for undefined variable expressions in the command line.

all:
	# When the command line is parsed during the initial call of
	# MainParseArgs, discardUndefined is still FALSE, therefore preserving
	# undefined subexpressions.  This makes sense because at that early
	# stage, almost no variables are defined.  On the other hand, the '='
	# assignment operator does not expand its right-hand side anyway.
	@echo 'The = assignment operator'
	@${.MAKE} -f ${MAKEFILE} print-undefined \
		CMDLINE='Undefined is $${UNDEFINED}.'
	@echo

	# The interesting case is using the ':=' assignment operator, which
	# expands its right-hand side.  But only those variables that are
	# defined.
	@echo 'The := assignment operator'
	@${.MAKE} -f ${MAKEFILE} print-undefined \
		CMDLINE:='Undefined is $${UNDEFINED}.'
	@echo

.if make(print-undefined)

.MAKEFLAGS: MAKEFLAGS_ASSIGN='Undefined is $${UNDEFINED}.'
.MAKEFLAGS: MAKEFLAGS_SUBST:='Undefined is $${UNDEFINED}.'

.info From the command line: ${CMDLINE}
.info From .MAKEFLAGS '=': ${MAKEFLAGS_ASSIGN}
.info From .MAKEFLAGS ':=': ${MAKEFLAGS_SUBST}

UNDEFINED?=	now defined

.info From the command line: ${CMDLINE}
.info From .MAKEFLAGS '=': ${MAKEFLAGS_ASSIGN}
.info From .MAKEFLAGS ':=': ${MAKEFLAGS_SUBST}

print-undefined:
.endif
