# $NetBSD: varparse-mod.mk,v 1.3 2026/08/22 19:18:44 rillig Exp $

# Tests for parsing expressions with modifiers.
#
# See also:
#	varmod.mk		For whether ':' is needed after a modifier.

# As of 2020-10-02, the below condition does not result in a parse error.
# The condition contains two separate mistakes.  The first mistake is that
# the :!cmd! modifier is missing the closing '!'.  The second mistake is that
# there is a stray '}' at the end of the whole condition.
#
# As of 2020-10-02, the actual parse result of this condition is a single
# expression with 2 modifiers. The first modifier is
# ":!echo "\$VAR"} !".  Afterwards, the parser optionally skips a ':' (at the
# bottom of ApplyModifiers) and continues with the next modifier, in this case
# "= "value"", which is interpreted as a SysV substitution modifier with an
# empty left-hand side, thereby appending the string " "value"" to each word
# of the expression.
#
# Some modifiers ensure that they are followed by either a ':' or the closing
# brace or parenthesis of the expression, see varmod.mk.
#
.if ${:!echo "\$VAR"} != "value"}
.endif

all:
	@:
