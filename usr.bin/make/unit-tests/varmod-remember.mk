# $NetBSD: varmod-remember.mk,v 1.4 2021/03/14 17:07:11 rillig Exp $
#
# Tests for the :_ modifier, which saves the current variable value
# in the _ variable or another, to be used later again.

.if ${1 2 3:L:_:@var@${_}@} != "1 2 3 1 2 3 1 2 3"
.  error
.endif

# In the parameterized form, having the variable name on the right side of
# the = assignment operator is confusing.  In almost all other situations
# the variable name is on the left-hand side of the = operator.  Luckily
# this modifier is only rarely needed.
.if ${1 2 3:L:@var@${var:_=SAVED:}@} != "1 2 3"
.  error
.elif ${SAVED} != "3"
.  error
.endif

all:
