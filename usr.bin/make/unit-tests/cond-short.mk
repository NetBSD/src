# $NetBSD: cond-short.mk,v 1.1 2020/06/28 09:42:40 rillig Exp $
#
# Demonstrates that in conditions, the right-hand side of an && or ||
# is evaluated even though it cannot influence the result.
#
# This is unexpected for several reasons:
#
# 1.  The manual page says: "bmake will only evaluate a conditional as
#     far as is necessary to determine its value."
#
# 2.  It differs from the way that these operators are evaluated in
#     almost all other programming languages.
#
# 3.  In cond.c there are lots of doEval variables.
#

.if 0 && ${echo "unexpected and" 1>&2 :L:sh}
.endif

.if 1 && ${echo "expected and" 1>&2 :L:sh}
.endif

.if 1 || ${echo "unexpected or" 1>&2 :L:sh}
.endif

.if 0 || ${echo "expected or" 1>&2 :L:sh}
.endif

# The following paragraphs demonstrate the workaround.

.if 0
.  if ${echo "unexpected nested and" 1>&2 :L:sh}
.  endif
.endif

.if 1
.elif ${echo "unexpected nested or" 1>&2 :L:sh}
.endif

all:
	@:;:
