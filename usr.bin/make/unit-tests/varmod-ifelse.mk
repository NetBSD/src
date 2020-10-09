# $NetBSD: varmod-ifelse.mk,v 1.4 2020/10/09 07:03:20 rillig Exp $
#
# Tests for the ${cond:?then:else} variable modifier, which evaluates either
# the then-expression or the else-expression, depending on the condition.

# TODO: Implementation

# When the :? is parsed, it is greedy.  The else branch spans all the
# text, up until the closing character '}', even if the text looks like
# another modifier.
.if ${1:?then:else:Q} != "then"
.  error
.endif
.if ${0:?then:else:Q} != "else:Q"
.  error
.endif

all:
	@:;
