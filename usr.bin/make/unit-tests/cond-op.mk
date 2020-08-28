# $NetBSD: cond-op.mk,v 1.3 2020/08/28 13:50:48 rillig Exp $
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

all:
	@:;
