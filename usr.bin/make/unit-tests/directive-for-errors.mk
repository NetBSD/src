# $NetBSD: directive-for-errors.mk,v 1.4 2023/05/08 10:24:07 rillig Exp $
#
# Tests for error handling in .for loops.

# A .for directive must be followed by whitespace, everything else results
# in a parse error.
.fori in 1 2 3
.  warning ${i}
.endfor

# A slash is not whitespace, therefore this is not parsed as a .for loop.
#
# XXX: The error message is misleading though.  As of 2020-12-31, it says
# "Unknown directive "for"", but that directive is actually known.  This is
# because ForEval does not detect the .for loop as such, so parsing
# continues in ParseLine > ParseDependencyLine > ParseDependency >
# ParseDependencyTargets > ParseErrorNoDependency, and there the directive
# name is parsed a bit differently.
.for/i in 1 2 3
.  warning ${i}
.endfor

# Before for.c 1.173 from 2023-05-08, the variable name could be an arbitrary
# word, it only needed to be separated by whitespace.  Even '$' and '\' were
# valid variable names, which was not useful in practice.
#
# The '$$' was not replaced with the values '1' or '3' from the .for loop,
# instead it was kept as-is, and when the .info directive expanded its
# argument, each '$$' got replaced with a single '$'.  The "long variable
# expression" ${$} got replaced though, even though this would be a parse
# error everywhere outside a .for loop.
${:U\$}=	dollar		# see whether the "variable" '$' is local
${:U\\}=	backslash	# see whether the "variable" '\' is local
.for $ \ in 1 2 3 4
.  info Dollar $$ ${$} $($) and backslash $\ ${\} $(\).
.endfor

# If there are no variables, there is no point in expanding the .for loop
# since this would end up in an endless loop, consuming 0 of the 3 values in
# each iteration.
.for in 1 2 3
# XXX: This should not be reached.  It should be skipped, as already done
# when the number of values is not a multiple of the number of variables,
# see below.
.  warning Should not be reached.
.endfor

# There are 3 variables and 5 values.  These 5 values cannot be split evenly
# among the variables, therefore the loop is not expanded at all, it is
# skipped instead.
.for a b c in 1 2 3 4 5
.  warning Should not be reached.
.endfor

# The list of values after the 'in' may be empty, no matter if this emptiness
# comes from an empty expansion or even from a syntactically empty line.
.for i in
.  info Would be reached if there were items to loop over.
.endfor

# A missing 'in' should parse the .for loop but skip the body.
# expect+1: missing `in' in for
.for i over k
# XXX: As of 2020-12-31, this line is reached once.
.  warning Should not be reached.
.endfor

# A malformed modifier should be detected and skip the body of the loop.
#
# XXX: As of 2020-12-31, Var_Subst doesn't report any errors, therefore
# the loop body is expanded as if no error had happened.
.for i in 1 2 ${:U3:Z} 4
.  warning Should not be reached.
.endfor
