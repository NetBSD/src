# $NetBSD: directive-undef.mk,v 1.8 2020/12/19 22:10:18 rillig Exp $
#
# Tests for the .undef directive.
#
# See also:
#	directive-misspellings.mk

# As of 2020-07-28, .undef only undefines the first variable.
# All further variable names are silently ignored.
# See parse.c, string literal "undef".
1=		1
2=		2
3=		3
.undef 1 2 3
.if ${1:U_}${2:U_}${3:U_} != _23
.  warning $1$2$3
.endif


# Without any arguments, until var.c 1.736 from 2020-12-19, .undef tried
# to delete the variable with the empty name, which never exists; see
# varname-empty.mk.  Since var.c 1.737 from 2020-12-19, .undef complains
# about a missing argument.
.undef


# Trying to delete the variable with the empty name is ok, it just won't
# ever do anything since that variable is never defined.
.undef ${:U}


# The argument of .undef is a single word, delimited by whitespace, without
# any possibility of escaping or having variable expressions containing
# spaces.  This word is then expanded exactly once, and the expanded string
# is the single variable name.  This allows variable names to contain spaces,
# as well as unbalanced single and double quotes.
1=		1
2=		2
3=		3
${:U1 2 3}=	one two three
VARNAMES=	1 2 3
.undef ${VARNAMES}		# undefines the variable "1 2 3"
.if defined(${:U1 2 3})
.  error
.endif
.if ${1}${2}${3} != "123"	# these are still defined
.  error
.endif
.undef 1
.undef 2
.undef 3


# It must be possible to undefine variables whose name includes spaces.
SPACE=		${:U }
${SPACE}=	space
.if !defined(${SPACE})
.  error
.endif
.undef ${SPACE}
.if defined(${SPACE})
.  error
.endif


# It must be possible to undefine variables whose name includes dollars.
DOLLAR=		$$
${DOLLAR}=	dollar
.if !defined(${DOLLAR})
.  error
.endif
.undef ${DOLLAR}
.if defined(${DOLLAR})
.  error
.endif


all:
	@:;
