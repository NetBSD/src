# $NetBSD: parse-var.mk,v 1.2 2022/08/06 21:26:05 rillig Exp $
#
# Tests for parsing variable expressions.

.MAKEFLAGS: -dL

# In variable assignments, there may be spaces in the middle of the left-hand
# side of the assignment, but only if they occur inside variable expressions.
# Leading spaces (but not tabs) are possible but unusual.
# Trailing spaces are common in some coding styles, others omit them.
VAR.${:U param }=	value
.if ${VAR.${:U param }} != "value"
.  error
.endif


# As of var.c 1.1027 from 2022-08-05, the exact way of parsing an expression
# depends on whether the expression is actually evaluated or merely parsed.
#
# If it is evaluated, nested expressions are parsed correctly, parsing each
# modifier according to its exact definition.  If the expression is merely
# parsed but not evaluated (because its value would not influence the outcome
# of the condition), and the expression contains a modifier, and that modifier
# contains a nested expression, the nested expression is not parsed
# correctly.  Instead, make only counts the opening and closing delimiters,
# which fails for nested modifiers with unbalanced braces.
#
# See ParseModifierPartDollar.

#.MAKEFLAGS: -dcpv
# Keep these braces outside the conditions below, to keep them simple to
# understand.  If the BRACE_PAIR had been replaced with ':U{}', the '}' would
# have to be escaped, but not the '{'.  This asymmetry would have made the
# example even more complicated to understand.
BRACE_PAIR=	{}
# In this test word, the '{{}' in the middle will be replaced.
BRACE_GROUP=	{{{{}}}}

# The inner ':S' modifier turns the word '{}' into '{{}'.
# The outer ':S' modifier then replaces '{{}' with '<lbraces>'.
# In the first case, the outer expression is relevant and is parsed correctly.
.if 1 && ${BRACE_GROUP:S,${BRACE_PAIR:S,{,{{,},<lbraces>,}
.endif
# In the second case, the outer expression is irrelevant.  In this case, in
# the parts of the outer ':S' modifier, make only counts the braces, and since
# the inner expression '${:U...}' contains more '{' than '}', parsing fails.
.if 0 && ${BRACE_GROUP:S,${BRACE_PAIR:S,{,{{,},<lbraces>,}
.endif
#.MAKEFLAGS: -d0


all: .PHONY
