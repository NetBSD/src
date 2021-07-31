# $NetBSD: varmod-order-numeric.mk,v 1.4 2021/07/31 20:55:46 rillig Exp $
#
# Tests for the :On variable modifier, which returns the words, sorted in
# ascending numeric order.

# This list contains only 32-bit numbers since the make code needs to conform
# to C90, which does not provide integer types larger than 32 bit.  It uses
# 'long long' by default, but that type is overridable if necessary.
# To get 53-bit integers even in C90, it would be possible to switch to
# 'double' instead, but that would allow floating-point numbers as well, which
# is out of scope for this variable modifier.
NUMBERS=	3 5 7 1 42 -42 5K -3m 1M 1k -2G

.if ${NUMBERS:On} != "-2G -3m -42 1 3 5 7 42 1k 5K 1M"
.  error ${NUMBERS:On}
.endif

.if ${NUMBERS:Orn} != "1M 5K 1k 42 7 5 3 1 -42 -3m -2G"
.  error ${NUMBERS:Orn}
.endif

# Both ':Onr' and ':Orn' have the same effect.
.if ${NUMBERS:Onr} != "1M 5K 1k 42 7 5 3 1 -42 -3m -2G"
.  error ${NUMBERS:Onr}
.endif

# If there are several numbers that have the same integer value, they are
# returned in unspecified order.
SAME_VALUE:=	${:U 79 80 0x0050 81 :On}
.if ${SAME_VALUE} != "79 80 0x0050 81" && ${SAME_VALUE} != "79 0x0050 80 81"
.  error ${SAME_VALUE}
.endif

# Hexadecimal and octal numbers are supported as well.
OCTAL=		0 010 0x7 9
.if ${OCTAL:On} != "0 0x7 010 9"
.  error ${OCTAL:On}
.endif

all:
