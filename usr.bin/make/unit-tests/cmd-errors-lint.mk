# $NetBSD: cmd-errors-lint.mk,v 1.5 2024/07/20 11:05:11 rillig Exp $
#
# Demonstrate how errors in expressions affect whether the commands
# are actually executed.

.MAKEFLAGS: -dL

all: undefined unclosed-expression unclosed-modifier unknown-modifier end

# Undefined variables in expressions are not an error.  They expand to empty
# strings.
undefined:
# expect: : undefined
	: $@ ${UNDEFINED}

unclosed-expression:
# expect: make: in target "unclosed-expression": Unclosed variable "UNCLOSED"
# expect-not: : unclosed-expression
	: $@ ${UNCLOSED

unclosed-modifier:
# expect: make: in target "unclosed-modifier": while evaluating variable "UNCLOSED" with value "": Unclosed expression, expecting '}'
# expect-not: : unclosed-modifier
	: $@ ${UNCLOSED:

unknown-modifier:
# expect: make: in target "unknown-modifier": while evaluating variable "UNKNOWN" with value "": Unknown modifier "Z"
# expect-not: : unknown-modifier
	: $@ ${UNKNOWN:Z}

end:
# expect: : end
	: $@

# expect: exit status 2
