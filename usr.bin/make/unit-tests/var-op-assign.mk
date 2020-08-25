# $NetBSD: var-op-assign.mk,v 1.3 2020/08/25 16:07:39 rillig Exp $
#
# Tests for the = variable assignment operator, which overwrites an existing
# variable or creates it.

# This is a simple variable assignment.
# To the left of the assignment operator '=' there is the variable name,
# and to the right is the variable value.
#
VAR=	value

# This condition demonstrates that whitespace around the assignment operator
# is discarded.  Otherwise the value would start with a single tab.
#
.if ${VAR} != "value"
.error
.endif

# Whitespace to the left of the assignment operator is ignored as well.
# The variable value can contain arbitrary characters.
#
# The '#' needs to be escaped with a backslash, this happens in a very
# early stage of parsing and applies to all line types, except for the
# commands, which are indented with a tab.
#
# The '$' needs to be escaped with another '$', otherwise it would refer to
# another variable.
#
VAR	=new value and \# some $$ special characters	# comment

# When a string literal appears in a condition, the escaping rules are
# different.  Run make with the -dc option to see the details.
.if ${VAR} != "new value and \# some \$ special characters"
.error ${VAR}
.endif

# The variable value may contain references to other variables.
# In this example, the reference is to the variable with the empty name,
# which always expands to an empty string.  This alone would not produce
# any side-effects, therefore the variable has a :!...! modifier that
# executes a shell command.
VAR=	${:! echo 'not yet evaluated' 1>&2 !}
VAR=	${:! echo 'this will be evaluated later' 1>&2 !}

# Now force the variable to be evaluated.
# This outputs the line to stderr.
.if ${VAR}
.endif

all:
	@:;
