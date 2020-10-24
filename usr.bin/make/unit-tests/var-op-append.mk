# $NetBSD: var-op-append.mk,v 1.6 2020/10/24 08:50:17 rillig Exp $
#
# Tests for the += variable assignment operator, which appends to a variable,
# creating it if necessary.

# Appending to an undefined variable is possible.
# The variable is created, and no extra space is added before the value.
VAR+=	one
.if ${VAR} != "one"
.  error
.endif

# Appending to an existing variable adds a single space and the value.
VAR+=	two
.if ${VAR} != "one two"
.  error
.endif

# Appending an empty string nevertheless adds a single space.
VAR+=	# empty
.if ${VAR} != "one two "
.  error
.endif

# Variable names may contain '+', and this character is also part of the
# '+=' assignment operator.  As far as possible, the '+' is interpreted as
# part of the assignment operator.
#
# See Parse_DoVar
C++=	value
.if ${C+} != "value" || defined(C++)
.  error
.endif

all:
	@:;
