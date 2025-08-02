# $NetBSD: cond-op-and-lint.mk,v 1.2.2.1 2025/08/02 05:58:31 perseant Exp $
#
# Tests for the && operator in .if conditions, in lint mode.

.MAKEFLAGS: -dL

# The '&' operator is not allowed in lint mode.
# It is not used in practice anyway.
# expect+1: Unknown operator "&"
.if 0 & 0
.  error
.else
.  error
.endif
