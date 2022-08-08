# $NetBSD: parse-var.mk,v 1.4 2022/08/08 19:53:28 rillig Exp $
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


# Since var.c 1.323 from 202-07-26 18:11 and before var.c 1.1028 from
# 2022-08-08, the exact way of parsing an expression depended on whether the
# expression was actually evaluated or merely parsed.
#
# If it was evaluated, nested expressions were parsed correctly, parsing each
# modifier according to its exact definition (see varmod.mk).
#
# If the expression was merely parsed but not evaluated (for example, because
# its value would not influence the outcome of the condition, or during the
# first pass of the ':@var@body@' modifier), and the expression contained a
# modifier, and that modifier contained a nested expression, the nested
# expression was not parsed correctly.  Instead, make only counted the opening
# and closing delimiters, which failed for nested modifiers with unbalanced
# braces.
#
# This naive brace counting was implemented in ParseModifierPartDollar.  As of
# var.c 1., there are still several other places that merely count braces
# instead of properly parsing subexpressions.

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
# In the second case, the outer expression was irrelevant.  In this case, in
# the parts of the outer ':S' modifier, make only counted the braces, and since
# the inner expression '${BRACE_PAIR:...}' contains more '{' than '}', parsing
# failed with the error message 'Unfinished modifier for "BRACE_GROUP"'.  Fixed
# in var.c 1.1028 from 2022-08-08.
.if 0 && ${BRACE_GROUP:S,${BRACE_PAIR:S,{,{{,},<lbraces>,}
.endif
#.MAKEFLAGS: -d0


all: .PHONY
