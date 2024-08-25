# $NetBSD: var-recursive.mk,v 1.7 2024/08/25 20:44:31 rillig Exp $
#
# Tests for expressions that refer to themselves and thus
# cannot be evaluated.

TESTS=	direct indirect conditional short target

# Since make exits immediately when it detects a recursive expression,
# the actual tests are run in sub-makes.
TEST?=	# none
.if ${TEST} == ""
all:
.for test in ${TESTS}
	@${.MAKE} -f ${MAKEFILE} TEST=${test} || echo "sub-exit status $$?"
.endfor

.elif ${TEST} == direct

DIRECT=	${DIRECT}	# Defining a recursive variable is not yet an error.
# expect+1: still there
.  info still there	# Therefore this line is printed.
# expect+1: while evaluating variable "DIRECT" with value "${DIRECT}": Variable DIRECT is recursive.
.  info ${DIRECT}	# But expanding the variable is an error.

.elif ${TEST} == indirect

# The chain of variables that refer to each other may be long.
INDIRECT1=	${INDIRECT2}
INDIRECT2=	${INDIRECT1}
# expect+1: while evaluating variable "INDIRECT1" with value "${INDIRECT2}": while evaluating variable "INDIRECT2" with value "${INDIRECT1}": Variable INDIRECT1 is recursive.
.  info ${INDIRECT1}

.elif ${TEST} == conditional

# The variable refers to itself, but only in the branch of a condition that
# is never satisfied and is thus not evaluated.
CONDITIONAL=	${1:?ok:${CONDITIONAL}}
# expect+1: ok
.  info ${CONDITIONAL}

.elif ${TEST} == short

# Short variable names can be expanded using the short-hand $V notation,
# which takes a different code path in Var_Parse for parsing the variable
# name.  Ensure that these are checked as well.
V=	$V
# expect+1: while evaluating variable "V" with value "$V": Variable V is recursive.
.  info $V

.elif ${TEST} == target

# If a recursive variable is accessed in a command of a target, the makefiles
# are not parsed anymore, so there is no location information from the
# .includes and .for directives.  In such a case, use the location of the last
# command of the target to provide at least a hint to the location.
VAR=	${VAR}
target:
	: OK
	: ${VAR}
	: OK

.else
.  error Unknown test "${TEST}"
.endif

all:
