# $NetBSD: opt-debug-var.mk,v 1.4 2025/01/10 23:00:38 rillig Exp $
#
# Tests for the -dv command line option, which adds debug logging about
# variable assignment and evaluation.

.MAKEFLAGS: -dv

# expect: Global: ASSIGNED = value
ASSIGNED=	value

# TODO: Explain why the empty assignment "Global: SUBST = " is needed.
# expect: Global: SUBST = # (empty)
# expect: Global: SUBST = value
SUBST:=		value

.if defined(ASSIGNED)
.endif

# The usual form of expressions is ${VAR}.  The form $(VAR) is used
# less often as it can be visually confused with the shell construct for
# capturing the output of a subshell, which looks the same.
#
# In conditions, a call to the function 'empty' is syntactically similar to
# the form $(VAR), only that the initial '$' is the 'y' of 'empty'.
#
# expect: Var_Parse: y(ASSIGNED) (eval)
.if !empty(ASSIGNED)
.endif


# An expression for a variable with a single-character ordinary name.
# expect: Var_Parse: $U (eval-defined)
# FIXME: Replace "Malformed" with "Undefined variable".
# expect+1: Malformed conditional '$U'
.if $U
.endif

# An expression for a target-specific variable with a single-character name.
# expect: Var_Parse: $< (eval-defined)
# FIXME: Replace "Malformed" with "Undefined variable".
# expect+1: Malformed conditional '$<'
.if $<
.endif


.MAKEFLAGS: -d0
