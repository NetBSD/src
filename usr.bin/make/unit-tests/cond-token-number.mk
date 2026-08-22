# $NetBSD: cond-token-number.mk,v 1.13 2026/08/22 07:54:46 rillig Exp $
#
# Tests for number tokens in .if conditions.
#
# TODO: Add introduction.

# 0 evaluates to false and can thus be used to comment out a section of code.
.if 0
.  error
.endif

# Even though -0 is a number that is accepted by strtod, it is not
# accepted by the condition parser.
#
# See the ch_isdigit call in CondParser_String.
# expect+1: Malformed conditional "-0"
.if -0
.  error
.else
.  error
.endif

# Even though +0 is a number that is accepted by strtod, it is not
# accepted by the condition parser.
#
# See the ch_isdigit call in CondParser_String.
# expect+1: Malformed conditional "+0"
.if +0
.  error
.else
.  error
.endif

# Even though -1 is a number that is accepted by strtod, it is not
# accepted by the condition parser.
#
# See the ch_isdigit call in CondParser_String.
# expect+1: Malformed conditional "!-1"
.if !-1
.  error
.else
.  error
.endif

# Even though +1 is a number that is accepted by strtod, it is not
# accepted by the condition parser.
#
# See the ch_isdigit call in CondParser_String.
# expect+1: Malformed conditional "!+1"
.if !+1
.  error
.else
.  error
.endif

# When the number comes from an expression though, it may be signed.
# XXX: This is inconsistent.
.if ${:U+0}
.  error
.endif

# When the number comes from an expression though, it may be signed.
# XXX: This is inconsistent.
.if !${:U+1}
.  error
.endif

# Hexadecimal numbers are accepted.
.if 0x0
.  error
.endif
.if 0x1
.else
.  error
.endif
HEX=	dead
.if 0x${HEX} == 57005 && 0xdead == 57005
.else
.  error
.endif

# Numbers with leading zeros are interpreted as decimal numbers, not octal.
.if 0777 == 777
.else
.  error
.endif

# This is not a hexadecimal number, even though it has an x.  It is
# interpreted as a string instead.  In a plain '.if', such a token evaluates
# to true if it is non-empty.  In other '.if' directives, such a token is
# evaluated as either of the functions defined(...) or make(...).
.if 3x4
.else
.  error
.endif

# Very small numbers round to 0.
.if 12345e-400
.  error
.endif
.if 12345e-200
.else
.  error
.endif

# Very large numbers round up to infinity on IEEE 754 implementations, or to
# the largest representable number (VAX); in particular, make does not fall
# back to checking whether a variable of that name is defined.
.if 12345e400
.else
.  error
.endif

all:
