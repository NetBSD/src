# $NetBSD: var-recursive.mk,v 1.1 2020/10/31 11:30:57 rillig Exp $
#
# Tests for variable expressions that refer to themselves and thus
# cannot be evaluated.

TESTS=	direct indirect conditional

# Since make exits immediately when it detects a recursive expression,
# the actual tests are run in sub-makes.
TEST?=	# none
.if ${TEST} == ""
all:
.for test in ${TESTS}
	@${.MAKE} -f ${MAKEFILE} TEST=${test} || :
.endfor

.elif ${TEST} == direct

DIRECT=	${DIRECT}	# Defining a recursive variable is not yet an error.
.  info still there	# Therefore this line is printed.
.  info ${DIRECT}	# But expanding the variable is an error.

.elif ${TEST} == indirect

# The chain of variables that refer to each other may be long.
INDIRECT1=	${INDIRECT2}
INDIRECT2=	${INDIRECT1}
.  info ${INDIRECT1}

.elif ${TEST} == conditional

# The variable refers to itself, but only in the branch of a condition that
# is never satisfied and is thus not evaluated.
CONDITIONAL=	${1:?ok:${CONDITIONAL}}
.  info ${CONDITIONAL}

.else
.  error Unknown test "${TEST}"
.endif

all:
