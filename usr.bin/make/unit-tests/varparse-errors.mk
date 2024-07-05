# $NetBSD: varparse-errors.mk,v 1.15 2024/07/05 17:41:50 rillig Exp $

# Tests for parsing and evaluating all kinds of expressions.
#
# This is the basis for redesigning the error handling in Var_Parse and
# Var_Subst, collecting typical and not so typical use cases.
#
# See also:
#	Var_Parse
#	Var_Subst

PLAIN=		plain value

LITERAL_DOLLAR=	To get a dollar, double $$ it.

INDIRECT=	An ${:Uindirect} value.

REF_UNDEF=	A reference to an ${UNDEF}undefined variable.

ERR_UNCLOSED=	An ${UNCLOSED expression.

ERR_BAD_MOD=	An ${:Uindirect:Z} expression with an unknown modifier.

ERR_EVAL=	An evaluation error ${:Uvalue:C,.,\3,}.

# In a conditional, an expression that is not enclosed in quotes is
# expanded using the mode VARE_EVAL_DEFINED.
# The variable itself must be defined.
# It may refer to undefined variables though.
.if ${REF_UNDEF} != "A reference to an undefined variable."
.  error
.endif

# As of 2020-12-01, errors in the variable name are silently ignored.
# Since var.c 1.754 from 2020-12-20, unknown modifiers at parse time result
# in an error message and a non-zero exit status.
# expect+1: while evaluating "${:U:Z}": Unknown modifier "Z"
VAR.${:U:Z}=	unknown modifier in the variable name
.if ${VAR.} != "unknown modifier in the variable name"
.  error
.endif

# As of 2020-12-01, errors in the variable name are silently ignored.
# Since var.c 1.754 from 2020-12-20, unknown modifiers at parse time result
# in an error message and a non-zero exit status.
# expect+1: while evaluating "${:U:Z}post": Unknown modifier "Z"
VAR.${:U:Z}post=	unknown modifier with text in the variable name
.if ${VAR.post} != "unknown modifier with text in the variable name"
.  error
.endif

# Demonstrate an edge case in which the 'static' for 'errorReported' in
# Var_Subst actually makes a difference, preventing "a plethora of messages".
# Given that this is an edge case and the error message is wrong and thus
# misleading anyway, that piece of code is probably not necessary.  The wrong
# condition was added in var.c 1.185 from 2014-05-19.
#
# To trigger this difference, the variable assignment must use the assignment
# operator ':=' to make VarEvalMode_ShouldKeepUndef return true.  There must
# be 2 expressions that create a parse error, which in this case is ':OX'.
# These expressions must be nested in some way.  The below expressions are
# minimal, that is, removing any part of it destroys the effect.
#
# Without the 'static', there would be one more message like this:
#	Undefined variable "${:U:OX"
#
#.MAKEFLAGS: -dv
IND=	${:OX}
# expect+6: while evaluating "${:U:OX:U${IND}} ${:U:OX:U${IND}}": Bad modifier ":OX"
# expect+5: while evaluating "${:U:OX:U${IND}}": Bad modifier ":OX"
# expect+4: Undefined variable "${:U:OX"
# expect+3: while evaluating variable "IND" with value "${:OX}": while evaluating "${:OX}": Bad modifier ":OX"
# expect+2: Undefined variable "${:U:OX"
# expect+1: while evaluating variable "IND" with value "${:OX}": while evaluating "${:OX}": Bad modifier ":OX"
_:=	${:U:OX:U${IND}} ${:U:OX:U${IND}}
#.MAKEFLAGS: -d0


# Before var.c 1.032 from 2022-08-24, make complained about 'Unknown modifier'
# or 'Bad modifier' when in fact the modifier was entirely correct, it was
# just not delimited by either ':' or '}' but instead by '\0'.
# expect: make: Unclosed expression, expecting '}' for modifier "Q" of variable "" with value ""
UNCLOSED:=	${:U:Q
# expect: make: Unclosed expression, expecting '}' for modifier "sh" of variable "" with value ""
UNCLOSED:=	${:U:sh
# expect: make: Unclosed expression, expecting '}' for modifier "tA" of variable "" with value ""
UNCLOSED:=	${:U:tA
# expect: make: Unclosed expression, expecting '}' for modifier "tsX" of variable "" with value ""
UNCLOSED:=	${:U:tsX
# expect: make: Unclosed expression, expecting '}' for modifier "ts" of variable "" with value ""
UNCLOSED:=	${:U:ts
# expect: make: Unclosed expression, expecting '}' for modifier "ts\040" of variable "" with value ""
UNCLOSED:=	${:U:ts\040
# expect: make: Unclosed expression, expecting '}' for modifier "u" of variable "" with value ""
UNCLOSED:=	${:U:u
# expect: make: Unclosed expression, expecting '}' for modifier "H" of variable "" with value "."
UNCLOSED:=	${:U:H
# expect: make: Unclosed expression, expecting '}' for modifier "[1]" of variable "" with value ""
UNCLOSED:=	${:U:[1]
# expect: make: Unclosed expression, expecting '}' for modifier "hash" of variable "" with value "b2af338b"
UNCLOSED:=	${:U:hash
# expect: make: Unclosed expression, expecting '}' for modifier "range" of variable "" with value "1"
UNCLOSED:=	${:U:range
# expect: make: Unclosed expression, expecting '}' for modifier "_" of variable "" with value ""
UNCLOSED:=	${:U:_
# expect: make: Unclosed expression, expecting '}' for modifier "gmtime" of variable "" with value "<timestamp>"
UNCLOSED:=	${:U:gmtime
# expect: make: Unclosed expression, expecting '}' for modifier "localtime" of variable "" with value "<timestamp>"
UNCLOSED:=	${:U:localtime
