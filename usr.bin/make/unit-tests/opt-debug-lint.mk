# $NetBSD: opt-debug-lint.mk,v 1.3 2020/09/13 20:21:24 rillig Exp $
#
# Tests for the -dL command line option, which runs additional checks
# to catch common mistakes, such as unclosed variable expressions.
 
.MAKEFLAGS: -dL

# Since 2020-09-13, undefined variables that are used on the left-hand side
# of a condition at parse time get a proper error message.  Before, the
# error message was "Malformed conditional" only, which was wrong and
# misleading.  The form of the condition is totally fine, it's the evaluation
# that fails.
#
# TODO: Get rid of the "Malformed conditional" error message.
# As long as the first error message is only printed in lint mode, it can
# get tricky to keep track of the actually printed error messages and those
# that still need to be printed.  That's probably a solvable problem though.
.if $X
.  error
.endif

# The dynamic variables like .TARGET are treated specially.  It does not make
# sense to expand them in the global scope since they will never be defined
# there under normal circumstances.  Therefore they expand to a string that
# will later be expanded correctly, when the variable is evaluated again in
# the scope of an actual target.
#
# Even though the "@" variable is not defined at this point, this is not an
# error.  In all practical cases, this is no problem.  This particular test
# case is made up and unrealistic.
.if $@ != "\$(.TARGET)"
.  error
.endif

# Since 2020-09-13, Var_Parse properly reports errors for undefined variables,
# but only in lint mode.  Before, it had only silently returned var_Error,
# hoping for the caller to print an error message.  This resulted in the
# well-known "Malformed conditional" error message, even though the
# conditional was well-formed and the only error was an undefined variable.
.if ${UNDEF}
.  error
.endif

all:
	@:;
