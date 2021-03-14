# $NetBSD: var-eval-short.mk,v 1.1 2021/03/14 11:49:37 rillig Exp $
#
# Tests for each variable modifier to ensure that they only do the minimum
# necessary computations.  If the result of the expression is not needed, they
# should only parse the modifier but not actually evaluate it.
#
# See also:
#	var.c, the comment starting with 'The ApplyModifier functions'
#	ApplyModifier, for the order of the modifiers
#	ParseModifierPart, for evaluating nested expressions
#	cond-short.mk

FAIL=	${:!echo unexpected 1>&2!}

# The following tests only ensure that nested expressions are not evaluated.
# They cannot ensure that any unexpanded text returned from ParseModifierPart
# is ignored as well.  To do that, it is necessary to step through the code of
# each modifier.

.if 0 && ${FAIL}
.endif

.if 0 && ${VAR::=${FAIL}}
.elif defined(VAR)
.  error
.endif

.if 0 && ${${FAIL}:?then:else}
.endif

.if 0 && ${1:?${FAIL}:${FAIL}}
.endif

.if 0 && ${0:?${FAIL}:${FAIL}}
.endif

.if 0 && ${:Uword:@${FAIL}@expr@}
.endif

.if 0 && ${:Uword:@var@${FAIL}@}
.endif

.if 0 && ${:Uword:[${FAIL}]}
.endif

.if 0 && ${:Uword:_=VAR}
.elif defined(VAR)
.  error
.endif

.if 0 && ${:Uword:C,${FAIL}****,,}
.endif

DEFINED=	# defined
.if 0 && ${DEFINED:D${FAIL}}
.endif

.if 0 && ${:Uword:E}
.endif

# As of 2021-03-14, the error 'Invalid time value: ${FAIL}}' is ok since
# ':gmtime' does not expand its argument.
.if 0 && ${:Uword:gmtime=${FAIL}}
.endif

.if 0 && ${:Uword:H}
.endif

.if 0 && ${:Uword:hash}
.endif

.if 0 && ${value:L}
.endif

# As of 2021-03-14, the error 'Invalid time value: ${FAIL}}' is ok since
# ':localtime' does not expand its argument.
.if 0 && ${:Uword:localtime=${FAIL}}
.endif

.if 0 && ${:Uword:M${FAIL}}
.endif

.if 0 && ${:Uword:N${FAIL}}
.endif

.if 0 && ${:Uword:O}
.endif

.if 0 && ${:Uword:Ox}
.endif

.if 0 && ${:Uword:P}
.endif

.if 0 && ${:Uword:Q}
.endif

.if 0 && ${:Uword:q}
.endif

.if 0 && ${:Uword:R}
.endif

.if 0 && ${:Uword:range}
.endif

.if 0 && ${:Uword:S,${FAIL},${FAIL},}
.endif

.if 0 && ${:Uword:sh}
.endif

.if 0 && ${:Uword:T}
.endif

.if 0 && ${:Uword:ts/}
.endif

.if 0 && ${:U${FAIL}}
.endif

.if 0 && ${:Uword:u}
.endif

.if 0 && ${:Uword:word=replacement}
.endif

all:
