# $NetBSD: cmd-errors-lint.mk,v 1.9 2026/08/22 19:18:44 rillig Exp $
#
# Demonstrate how errors in expressions affect whether the commands
# are actually executed, in strict mode (-dL).

.MAKEFLAGS: -dL

all: undefined unclosed-expression unclosed-modifier unknown-modifier end

# Undefined variables in expressions are not an error.  They expand to empty
# strings.
undefined:
# expect: : undefined .
	: $@ ${UNDEFINED}.

unclosed-expression:
# expect: make: Unclosed variable "UNCLOSED"
# expect-not: : unclosed-expression
	: $@ ${UNCLOSED

unclosed-modifier:
# expect: make: Unclosed expression, expecting "}"
# expect-not: : unclosed-modifier
	: $@ ${UNCLOSED:

unknown-modifier:
# expect: make: Unknown modifier ":Z"
# expect-not: : unknown-modifier
	: $@ ${UNKNOWN:Z}

end:
# expect: : end
	: $@

# expect: exit status 2
