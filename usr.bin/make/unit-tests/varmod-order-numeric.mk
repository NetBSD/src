# $NetBSD: varmod-order-numeric.mk,v 1.1 2021/07/30 19:55:22 sjg Exp $
#
# Tests for the :On variable modifier, which returns the words, sorted in
# ascending numeric order.

NUMBERS=	3 5 7 1 42 -42 1M 1k

.if ${NUMBERS:On} != "-42 1 3 5 7 42 1k 1M"
.  error ${NUMBERS:On}
.endif

.if ${NUMBERS:Orn} != "1M 1k 42 7 5 3 1 -42"
.  error ${NUMBERS:Orn}
.endif


all:
	@:;
