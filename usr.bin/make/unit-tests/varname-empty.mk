# $NetBSD: varname-empty.mk,v 1.3 2020/08/22 21:02:56 rillig Exp $
#
# Tests for the special variable with the empty name.
#
# The variable "" is not supposed to be assigned any value.
# This is because it is heavily used in the .for loop expansion,
# as well as to generate arbitrary strings, as in ${:Ufallback}.

# Until 2020-08-22 it was possible to assign a value to the variable with
# the empty name, leading to all kinds of unexpected effects.
?=	default
#=	assigned	# XXX: probably undefined behavior
+=	appended
:=	subst
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
