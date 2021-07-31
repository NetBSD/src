# $NetBSD: varmod-order.mk,v 1.6 2021/07/31 20:55:46 rillig Exp $
#
# Tests for the :O variable modifier and its variants, which either sort the
# words of the value or shuffle them.

NUMBERS=	one two three four five six seven eight nine ten

.if ${NUMBERS:O} != "eight five four nine one seven six ten three two"
.  error ${NUMBERS:O}
.endif

# Unknown modifier "OX"
_:=	${NUMBERS:OX}

# Unknown modifier "OxXX"
_:=	${NUMBERS:OxXX}

# Missing closing brace, to cover the error handling code.
_:=	${NUMBERS:O
_:=	${NUMBERS:On
_:=	${NUMBERS:Onr

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

all:
