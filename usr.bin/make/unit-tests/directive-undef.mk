# $NetBSD: directive-undef.mk,v 1.7 2020/12/19 20:35:39 rillig Exp $
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

# Without any arguments, .undef tries to delete the variable with the empty
# name, which never exists; see varname-empty.mk.
.undef				# oops: missing argument


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
