# $NetBSD: cmd-errors-jobs.mk,v 1.3 2024/04/20 10:18:55 rillig Exp $
#
# Demonstrate how errors in variable expansions affect whether the commands
# are actually executed in jobs mode.

.MAKEFLAGS: -j1

all: undefined unclosed-variable unclosed-modifier unknown-modifier end

# Undefined variables are not an error.  They expand to empty strings.
# expect: : undefined--eol
undefined:
	: $@-${UNDEFINED}-eol

# XXX: This command is executed even though it contains parse errors.
# expect: make: in target "unclosed-variable": Unclosed variable "UNCLOSED"
# expect: : unclosed-variable-
unclosed-variable:
	: $@-${UNCLOSED

# XXX: This command is executed even though it contains parse errors.
# expect: make: Unclosed expression, expecting '}' for "UNCLOSED"
# expect: : unclosed-modifier-
unclosed-modifier:
	: $@-${UNCLOSED:

# XXX: This command is executed even though it contains parse errors.
# expect: make: in target "unknown-modifier": while evaluating variable "UNKNOWN": Unknown modifier "Z"
# expect: : unknown-modifier--eol
unknown-modifier:
	: $@-${UNKNOWN:Z}-eol

# expect: : end-eol
end:
	: $@-eol

# XXX: Despite the parse errors, the exit status is 0.
# expect: exit status 0
