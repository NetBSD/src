# $NetBSD: varmod-ifelse.mk,v 1.3 2020/10/02 20:34:59 rillig Exp $
#
# Tests for the ${cond:?then:else} variable modifier, which evaluates either
# the then-expression or the else-expression, depending on the condition.

# TODO: Implementation

# TODO: Test another modifier after ifelse; does not work, it becomes part
# of the else branch.

all:
	@:;
