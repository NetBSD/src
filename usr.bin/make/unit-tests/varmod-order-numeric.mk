# $NetBSD: varmod-order-numeric.mk,v 1.3 2021/07/30 23:28:04 rillig Exp $
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

# Shuffling numerically doesn't make sense, so don't allow 'x' and 'n' to be
# combined.
#
# expect-text: Bad modifier ":Oxn" for variable "NUMBERS"
# expect+1: Malformed conditional (${NUMBERS:Oxn})
.if ${NUMBERS:Oxn}
.  error
.else
.  error
.endif

# Extra characters after ':On' are detected and diagnosed.
# TODO: Add line number information to the "Bad modifier" diagnostic.
#
# expect-text: Bad modifier ":On_typo" for variable "NUMBERS"
.if ${NUMBERS:On_typo}
.  error
.else
.  error
.endif

# Extra characters after ':Onr' are detected and diagnosed.
#
# expect-text: Bad modifier ":Onr_typo" for variable "NUMBERS"
.if ${NUMBERS:Onr_typo}
.  error
.else
.  error
.endif

# Extra characters after ':Orn' are detected and diagnosed.
#
# expect+1: Bad modifier ":Orn_typo" for variable "NUMBERS"
.if ${NUMBERS:Orn_typo}
.  error
.else
.  error
.endif

# Repeating the 'n' is not supported.  In the typical use cases, the sorting
# criteria are fixed, not computed, therefore allowing this redundancy does
# not make sense.
#
# expect-text: Bad modifier ":Onn" for variable "NUMBERS"
.if ${NUMBERS:Onn}
.  error
.else
.  error
.endif

# Repeating the 'r' is not supported as well, for the same reasons as above.
#
# expect-text: Bad modifier ":Onrr" for variable "NUMBERS"
.if ${NUMBERS:Onrr}
.  error
.else
.  error
.endif

# Repeating the 'r' is not supported as well, for the same reasons as above.
#
# expect-text: Bad modifier ":Orrn" for variable "NUMBERS"
.if ${NUMBERS:Orrn}
.  error
.else
.  error
.endif

# Missing closing brace, to cover the error handling code.
_:=	${NUMBERS:O
_:=	${NUMBERS:On
_:=	${NUMBERS:Onr

all:
