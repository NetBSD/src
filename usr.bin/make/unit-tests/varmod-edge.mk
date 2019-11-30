# $NetBSD: varmod-edge.mk,v 1.3 2019/11/30 02:55:47 rillig Exp $
#
# Tests for edge cases in variable modifiers.
#
# These tests demonstrate the current implementation in small examples.
# They may contain surprising behavior.
#
# Each test consists of:
# - INP, the input to the test
# - MOD, the expression for testing the modifier
# - EXP, the expected output

TESTS+=		M-paren
INP.M-paren=	(parentheses) {braces} (opening closing) ()
MOD.M-paren=	${INP.M-paren:M(*)}
EXP.M-paren=	(parentheses) ()

# The first closing brace matches the opening parenthesis.
# The second closing brace actually ends the variable expression.
#
# XXX: This is unexpected but rarely occurs in practice.
TESTS+=		M-mixed
INP.M-mixed=	(paren-brace} (
MOD.M-mixed=	${INP.M-mixed:M(*}}
EXP.M-mixed=	(paren-brace}

# When the :M and :N modifiers are parsed, the pattern finishes as soon
# as open_parens + open_braces == closing_parens + closing_braces. This
# means that ( and } form a matching pair.
#
# Nested variable expressions are not parsed as such. Instead, only the
# parentheses and braces are counted. This leads to a parse error since
# the nested expression is not "${:U*)}" but only "${:U*)", which is
# missing the closing brace. The expression is evaluated anyway.
# The final brace in the output comes from the end of M.nest-mix.
#
# XXX: This is unexpected but rarely occurs in practice.
TESTS+=		M-nest-mix
INP.M-nest-mix=	(parentheses)
MOD.M-nest-mix=	${INP.M-nest-mix:M${:U*)}}
EXP.M-nest-mix=	(parentheses)}

# In contrast to parentheses and braces, the brackets are not counted
# when the :M modifier is parsed since Makefile variables only take the
# ${VAR} or $(VAR) forms, but not $[VAR].
#
# The final ] in the pattern is needed to close the character class.
TESTS+=		M-nest-brk
INP.M-nest-brk=	[ [[ [[[
MOD.M-nest-brk=	${INP.M-nest-brk:M${:U[[[[[]}}
EXP.M-nest-brk=	[

# The pattern in the nested variable has an unclosed character class.
# No error is reported though, and the pattern is closed implicitly.
#
# XXX: It is unexpected that no error is reported.
# See str.c, function Str_Match.
TESTS+=		M-pat-err
INP.M-pat-err=	[ [[ [[[
MOD.M-pat-err=	${INP.M-pat-err:M${:U[[}}
EXP.M-pat-err=	[

# The first backslash does not escape the second backslash.
# Therefore, the second backslash escapes the parenthesis.
# This means that the pattern ends there.
# The final } in the output comes from the end of MOD.M-bsbs.
#
# If the first backslash were to escape the second backslash, the first
# closing brace would match the opening parenthesis (see M-mixed), and
# the second closing brace would be needed to close the variable.
# After that, the remaining backslash would escape the parenthesis in
# the pattern, therefore (} would match.
TESTS+=		M-bsbs
INP.M-bsbs=	(} \( \(}
MOD.M-bsbs=	${INP.M-bsbs:M\\(}}
EXP.M-bsbs=	\(}
#EXP.M-bsbs=	(}	# If the first backslash were to escape ...

all:
.for test in ${TESTS}
.  if ${MOD.${test}} == ${EXP.${test}}
	@printf 'ok %s\n' ${test:Q}''
.  else
	@printf 'error in %s: expected %s, got %s\n' \
		${test:Q}'' ${EXP.${test}:Q}'' ${MOD.${test}:Q}''
.  endif
.endfor
