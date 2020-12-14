# $NetBSD: cond-eof.mk,v 1.1 2020/12/14 20:23:50 rillig Exp $
#
# Tests for parsing conditions, especially the end of such conditions, which
# are represented as the token TOK_EOF.

SIDE_EFFECT=	${:!echo 'side effect' 1>&2!}

# In the following conditions, ${SIDE_EFFECT} is the position of the first
# parse error.  It is always fully evaluated, even if it were not necessary
# to expand the variable expression.  This is because these syntax errors are
# an edge case that does not occur during normal operation, therefore there
# is no need to optimize for this case, and it would slow down the common
# case as well.
.if 0 ${SIDE_EFFECT}
.endif
.if 1 ${SIDE_EFFECT}
.endif
.if (0) ${SIDE_EFFECT}
.endif
