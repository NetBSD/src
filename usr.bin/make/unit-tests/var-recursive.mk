# $NetBSD: var-recursive.mk,v 1.9 2024/08/27 05:01:03 rillig Exp $
#
# Tests for expressions that refer to themselves and thus cannot be
# evaluated, as that would lead to an endless loop.

.if make(loadtime)

DIRECT=	${DIRECT}	# Defining a recursive variable is not an error.
# expect+2: while evaluating variable "DIRECT" with value "${DIRECT}": Variable DIRECT is recursive.
# expect+1: <>
.  info <${DIRECT}>	# But expanding such a variable is an error.


# The chain of variables that refer to each other may be long.
INDIRECT1=	${INDIRECT2}
INDIRECT2=	${INDIRECT1}
# expect+2: while evaluating variable "INDIRECT1" with value "${INDIRECT2}": while evaluating variable "INDIRECT2" with value "${INDIRECT1}": Variable INDIRECT1 is recursive.
# expect+1: <>
.  info <${INDIRECT1}>


# The variable refers to itself, but only in the branch of a condition that
# is not satisfied and is thus not evaluated.
CONDITIONAL=	${1:?ok:${CONDITIONAL}}
# expect+1: <ok>
.  info <${CONDITIONAL}>


# An expression with modifiers is skipped halfway.  This can lead to wrong
# follow-up error messages, but recursive variables occur seldom.
MODIFIERS=	${MODIFIERS:Mpattern}
# expect+2: while evaluating variable "MODIFIERS" with value "${MODIFIERS:Mpattern}": Variable MODIFIERS is recursive.
# expect+1: <Mpattern}>
.  info <${MODIFIERS}>


# Short variable names can be expanded using the short-hand $V notation,
# which takes a different code path in Var_Parse for parsing the variable
# name.  Ensure that these are checked as well.
V=	$V
# expect+2: while evaluating variable "V" with value "$V": Variable V is recursive.
# expect+1: <>
.  info <$V>

.elif make(runtime)

VAR=	${VAR}
runtime:
# expect: : before-recursive
	: before-recursive
# expect: make: in target "runtime": while evaluating variable "VAR" with value "${VAR}": Variable VAR is recursive.
# expect-not: recursive-line-before
# expect-not: recursive-line-after
	: recursive-line-before <${VAR}> recursive-line-after
# expect-not: after-recursive
	: after-recursive

.else

all:
	@${MAKE} -f ${MAKEFILE} loadtime || echo "sub-exit status $$?"
	@${MAKE} -f ${MAKEFILE} runtime || echo "sub-exit status $$?"

.endif
