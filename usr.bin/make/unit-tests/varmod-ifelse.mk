# $NetBSD: varmod-ifelse.mk,v 1.15 2021/04/19 23:43:14 rillig Exp $
#
# Tests for the ${cond:?then:else} variable modifier, which evaluates either
# the then-expression or the else-expression, depending on the condition.
#
# The modifier was added on 1998-04-01.
#
# Until 2015-10-11, the modifier always evaluated both the "then" and the
# "else" expressions.

# TODO: Implementation

# The variable name of the expression is expanded and then taken as the
# condition.  In this case it becomes:
#
#	variable expression == "variable expression"
#
# This confuses the parser, which expects an operator instead of the bare
# word "expression".  If the name were expanded lazily, everything would be
# fine since the condition would be:
#
#	${:Uvariable expression} == "literal"
#
# Evaluating the variable name lazily would require additional code in
# Var_Parse and ParseVarname, it would be more useful and predictable
# though.
.if ${${:Uvariable expression} == "literal":?bad:bad}
.  error
.else
.  error
.endif

# In a variable assignment, undefined variables are not an error.
# Because of the early expansion, the whole condition evaluates to
# ' == ""' though, which cannot be parsed because the left-hand side looks
# empty.
COND:=	${${UNDEF} == "":?bad-assign:bad-assign}

# In a condition, undefined variables generate a "Malformed conditional"
# error.  That error message is wrong though.  In lint mode, the correct
# "Undefined variable" error message is generated.
# The difference to the ':=' variable assignment is the additional
# "Malformed conditional" error message.
.if ${${UNDEF} == "":?bad-cond:bad-cond}
.  error
.else
.  error
.endif

# When the :? is parsed, it is greedy.  The else branch spans all the
# text, up until the closing character '}', even if the text looks like
# another modifier.
.if ${1:?then:else:Q} != "then"
.  error
.endif
.if ${0:?then:else:Q} != "else:Q"
.  error
.endif

# This line generates 2 error messages.  The first comes from evaluating the
# malformed conditional "1 == == 2", which is reported as "Bad conditional
# expression" by ApplyModifier_IfElse.  The variable expression containing that
# conditional therefore returns a parse error from Var_Parse, and this parse
# error propagates to CondEvalExpression, where the "Malformed conditional"
# comes from.
.if ${1 == == 2:?yes:no} != ""
.  error
.else
.  error
.endif

# If the "Bad conditional expression" appears in a quoted string literal, the
# error message "Malformed conditional" is not printed, leaving only the "Bad
# conditional expression".
#
# XXX: The left-hand side is enclosed in quotes.  This results in Var_Parse
# being called without VARE_UNDEFERR.  When ApplyModifier_IfElse
# returns AMR_CLEANUP as result, Var_Parse returns varUndefined since the
# value of the variable expression is still undefined.  CondParser_String is
# then supposed to do proper error handling, but since varUndefined is local
# to var.c, it cannot distinguish this return value from an ordinary empty
# string.  The left-hand side of the comparison is therefore just an empty
# string, which is obviously equal to the empty string on the right-hand side.
#
# XXX: The debug log for -dc shows a comparison between 1.0 and 0.0.  The
# condition should be detected as being malformed before any comparison is
# done since there is no well-formed comparison in the condition at all.
.MAKEFLAGS: -dc
.if "${1 == == 2:?yes:no}" != ""
.  error
.else
.  warning Oops, the parse error should have been propagated.
.endif
.MAKEFLAGS: -d0

# As of 2020-12-10, the variable "name" is first expanded, and the result of
# this expansion is then taken as the condition.  To force the variable
# expression in the condition to be evaluated at exactly the right point,
# the '$' of the intended '${VAR}' escapes from the parser in form of the
# expression ${:U\$}.  Because of this escaping, the variable "name" and thus
# the condition ends up as "${VAR} == value", just as intended.
#
# This hack does not work for variables from .for loops since these are
# expanded at parse time to their corresponding ${:Uvalue} expressions.
# Making the '$' of the '${VAR}' expression indirect hides this expression
# from the parser of the .for loop body.  See ForLoop_SubstVarLong.
.MAKEFLAGS: -dc
VAR=	value
.if ${ ${:U\$}{VAR} == value :?ok:bad} != "ok"
.  error
.endif
.MAKEFLAGS: -d0

# On 2021-04-19, when building external/bsd/tmux with HAVE_LLVM=yes and
# HAVE_GCC=no, the following conditional generated this error message:
#
#	make: Bad conditional expression 'string == "literal" && no >= 10'
#	    in 'string == "literal" && no >= 10?yes:no'
#
# Despite the error message (which was not clearly marked with "error:"),
# the build continued, for historical reasons, see main_Exit.
#
# The tricky detail here is that the condition that looks so obvious in the
# form written in the makefile becomes tricky when it is actually evaluated.
# This is because the condition is written in the place of the variable name
# of the expression, and in an expression, the variable name is always
# expanded first, before even looking at the modifiers.  This happens for the
# modifier ':?' as well, so when CondEvalExpression gets to see the
# expression, it already looks like this:
#
#	string == "literal" && no >= 10
#
# When parsing such an expression, the parser used to be strict.  It first
# evaluated the left-hand side of the operator '&&' and then started parsing
# the right-hand side 'no >= 10'.  The word 'no' is obviously a string
# literal, not enclosed in quotes, which is ok, even on the left-hand side of
# the comparison operator, but only because this is a condition in the
# modifier ':?'.  In an ordinary directive '.if', this would be a parse error.
# For strings, only the comparison operators '==' and '!=' are defined,
# therefore parsing stopped at the '>', producing the 'Bad conditional
# expression'.
#
# TODO: make should at least describe the part of the condition that is
#  wrong. In this case it is probably the "no >= 10".  Ideally that should
#  not matter though since the left-hand side of the '&&' evaluates to false,
#  thus the right-hand side only needs to be parsed, not evaluated.  Since
#  this is the modifier ':?', which expands subexpressions before parsing
#  the condition, the "no >= 10" is probably a parse error since it "can be
#  seen at compile-time" that the operand types of '>=' don't match.  Only
#  that the concept of "compile-time" does not really apply here.
STRING=		string
NUMBER=		no		# not really a number
.info ${${STRING} == "literal" && ${NUMBER} >= 10:?yes:no}.
.info ${${STRING} == "literal" || ${NUMBER} >= 10:?yes:no}.

# The following situation occasionally occurs with MKINET6 or similar
# variables.
NUMBER=		# empty, not really a number either
.info ${${STRING} == "literal" && ${NUMBER} >= 10:?yes:no}.
.info ${${STRING} == "literal" || ${NUMBER} >= 10:?yes:no}.
