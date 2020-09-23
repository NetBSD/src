# $NetBSD: counter-append.mk,v 1.1 2020/09/23 03:33:55 rillig Exp $
#
# Demonstrates that it is not easily possible to let make count
# the number of times a variable is actually accessed, using the
# ::+= variable modifier.
#
# As of 2020-09-23, the counter ends up at having 6 words, even
# though the NEXT variable is only accessed 3 times.  This is
# surprising.
#
# A hint to this surprising behavior is that the variables don't
# get fully expanded.  For example, A does not simply contain the
# value "1" but an additional unexpanded ${COUNTER:...} before it.

.MAKEFLAGS: -dv

RELEVANT=	yes (load-time part)	# just to filter the output

COUNTER=	# zero

NEXT=		${COUNTER::+=a}${COUNTER:[#]}

# This variable is first set to empty and then expanded.
# See parse.c, function Parse_DoVar, keyword "!Var_Exists".
A:=		${NEXT}
B:=		${NEXT}
C:=		${NEXT}

RELEVANT=	no

all:
	@: ${RELEVANT::=yes (run-time part)}
	@echo A=${A:Q} B=${B:Q} C=${C:Q} COUNTER=${COUNTER:[#]:Q}
	@: ${RELEVANT::=no}
