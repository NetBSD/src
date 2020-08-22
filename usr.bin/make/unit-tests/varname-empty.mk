# $NetBSD: varname-empty.mk,v 1.1 2020/08/22 20:23:14 rillig Exp $
#
# Tests for the special variable with the empty name.
#
# The variable "" is not supposed to be assigned any value.
# This is because it is heavily used in the .for loop expansion,
# as well as to generate arbitrary strings, as in ${:Ufallback}.

# Oops.
!=	echo 'value'

# The .for loop expands the expression ${i} to ${:U1}, ${:U2} and so on.
# This only works if the variable with the empty name is guaranteed to
# be undefined.
.for i in 1 2 3
NUMBERS+=	${i}
.endfor

all:
	@echo ${:Ufallback}
	@echo ${NUMBERS}
