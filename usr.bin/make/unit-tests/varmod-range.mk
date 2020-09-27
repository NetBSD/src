# $NetBSD: varmod-range.mk,v 1.4 2020/09/27 18:11:31 rillig Exp $
#
# Tests for the :range variable modifier, which generates sequences
# of integers from the given range.

# The :range modifier generates a sequence of integers, one number per
# word of the variable expression's value.
.if ${a b c:L:range} != "1 2 3"
.  error
.endif

# To preserve spaces in a word, they can be enclosed in quotes, just like
# everywhere else.
.if ${:U first "the second word" third 4 :range} != "1 2 3 4"
.  error
.endif

# The :range modifier takes the number of words from the value of the
# variable expression.  If that expression is undefined, the range is
# undefined as well.  This should not come as a surprise.
.if "${:range}" != ""
.  error
.endif

# The :range modifier can be given a parameter, which makes the generated
# range independent from the value or the name of the variable expression.
#
# XXX: As of 2020-09-27, the :range=... modifier does not turn the undefined
# expression into a defined one.  This looks like an oversight.
.if "${:range=5}" != ""
.  error
.endif

all:
	@echo ${a b c:L:rang}			# modifier name too short
	@echo ${a b c:L:range}			# ok
	@echo ${a b c:L:rango}			# misspelled
	@echo ${a b c:L:ranger}			# modifier name too long
