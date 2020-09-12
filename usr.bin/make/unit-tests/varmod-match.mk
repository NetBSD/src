# $NetBSD: varmod-match.mk,v 1.4 2020/09/12 22:35:43 rillig Exp $
#
# Tests for the :M variable modifier, which filters words that match the
# given pattern.
#
# See ApplyModifier_Match and ModifyWord_Match for the implementation.

.MAKEFLAGS: -dc

NUMBERS=	One Two Three Four five six seven

# Only keep numbers that start with an uppercase letter.
.if ${NUMBERS:M[A-Z]*} != "One Two Three Four"
.  error
.endif

# Only keep numbers that don't start with an uppercase letter.
.if ${NUMBERS:M[^A-Z]*} != "five six seven"
.  error
.endif

# Only keep numbers that don't start with s and at the same time
# ends with either of [ex].
.if ${NUMBERS:M[^s]*[ex]} != "One Three five"
.  error
.endif

# Before 2020-06-13, this expression took quite a long time in Str_Match,
# calling itself 601080390 times for 16 asterisks.
.if ${:U****************:M****************b}
.endif

# To match a dollar sign in a word, double it.
# This is different from the :S and :C variable modifiers, where a '$'
# has to be escaped as '$$'.
.if ${:Ua \$ sign:M*$$*} != "\$"
.  error
.endif

# In the :M modifier, it does not work to escape a dollar using a backslash.
# This is different from the :S, :C and a few other variable modifiers.
${:U*}=		asterisk
.if ${:Ua \$ sign any-asterisk:M*\$*} != "any-asterisk"
.  error
.endif

all:
	@:;
