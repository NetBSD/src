# $NetBSD: directive-endif.mk,v 1.4 2020/12/14 20:57:31 rillig Exp $
#
# Tests for the .endif directive.
#
# See also:
#	Cond_EvalLine

# TODO: Implementation

.MAKEFLAGS: -dL

# Error: .endif does not take arguments
# XXX: Missing error message
.if 0
.endif 0

# Error: .endif does not take arguments
# XXX: Missing error message
.if 1
.endif 1

# Comments are allowed after an '.endif'.
.if 2
.endif # comment

# Only whitespace and comments are allowed after an '.endif', but nothing
# else.
# XXX: Missing error message
.if 1
.endif0

# Only whitespace and comments are allowed after an '.endif', but nothing
# else.
# XXX: Missing error message
.if 1
.endif/

# After an '.endif', no other letter must occur.  This 'endifx' is not
# parsed as an 'endif', therefore another '.endif' must follow to balance
# the directives.
.if 1
.endifx
.endif # to close the preceding '.if'

all:
	@:;
