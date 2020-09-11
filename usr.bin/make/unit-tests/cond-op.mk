# $NetBSD: cond-op.mk,v 1.6 2020/09/11 05:12:08 rillig Exp $
#
# Tests for operators like &&, ||, ! in .if conditions.
#
# See also:
#	cond-op-and.mk
#	cond-op-not.mk
#	cond-op-or.mk
#	cond-op-parentheses.mk

# In make, && binds more tightly than ||, like in C.
# If make had the same precedence for both && and ||, the result would be
# different.
# If || were to bind more tightly than &&, the result would be different
# as well.
.if !(1 || 1 && 0)
.error
.endif

# If make were to interpret the && and || operators like the shell, the
# implicit binding would be this:
.if (1 || 1) && 0
.error
.endif

# The precedence of the ! operator is different from C though. It has a
# lower precedence than the comparison operators.
.if !"word" == "word"
.error
.endif

# This is how the above condition is actually interpreted.
.if !("word" == "word")
.error
.endif

# TODO: Demonstrate that the precedence of the ! and == operators actually
# makes a difference.  There is a simple example for sure, I just cannot
# wrap my head around it.

# This condition is malformed because the '!' on the right-hand side must not
# appear unquoted.  If any, it must be enclosed in quotes.
# In any case, it is not interpreted as a negation of an unquoted string.
# See CondParser_String.
.if "!word" == !word
.error
.endif

# Surprisingly, the ampersand and pipe are allowed in bare strings.
# That's another opportunity for writing confusing code.
# See CondParser_String, which only has '!' in the list of stop characters.
.if "a&&b||c" != a&&b||c
.error
.endif

# As soon as the parser sees the '$', it knows that the condition will
# be malformed.  Therefore there is no point in evaluating it.
# As of 2020-09-11, that part of the condition is evaluated nevertheless.
.if 0 ${ERR::=evaluated}
.  error
.endif
.if ${ERR:Uundefined} == evaluated
.  warning After detecting a parse error, the rest is evaluated.
.endif

# Just in case that parsing should ever stop on the first error.
.info Parsing continues until here.

all:
	@:;
