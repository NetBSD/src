# $NetBSD: varname-empty.mk,v 1.6 2020/09/24 06:03:44 rillig Exp $
#
# Tests for the special variable with the empty name.
#
# There is no variable named "" at all, and this fact is used a lot in
# variable expressions of the form ${:Ufallback}.  These expressions are
# based on the variable named "" and use the :U modifier to assign a
# fallback value to the expression (but not to the variable).
#
# This form of expressions is used to implement value substitution in the
# .for loops.  Another use case is in a variable assignment of the form
# ${:Uvarname}=value, which allows for characters in the variable name that
# would otherwise be interpreted by the parser, such as whitespace, ':',
# '=', '$', backslash.
#
# The only places where a variable is assigned a value are Var_Set and
# Var_Append, and these places protect the variable named "" from being
# defined.  This is different from read-only variables, as that flag can
# only apply to variables that are defined.
#
# This is because it is heavily used in the .for loop expansion,
# as well as to generate arbitrary strings, as in ${:Ufallback}.

# Until 2020-08-22 it was possible to assign a value to the variable with
# the empty name, leading to all kinds of unexpected effects.
#
# Before 2020-08-22, the simple assignment operator '=' had an off-by-one
# bug that caused unrelated memory to be read in Parse_DoVar, invoking
# undefined behavior.
?=	default
=	assigned	# undefined behavior until 2020-08-22
+=	appended
:=	subst
!=	echo 'shell-output'

# The .for loop expands the expression ${i} to ${:U1}, ${:U2} and so on.
# This only works if the variable with the empty name is guaranteed to
# be undefined.
.for i in 1 2 3
NUMBERS+=	${i}
.endfor

all:
	@echo out: ${:Ufallback}
	@echo out: ${NUMBERS}
