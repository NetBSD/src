# $NetBSD: modmatch.mk,v 1.7 2020/08/01 18:59:16 rillig Exp $
#
# Tests for the :M and :S modifiers.

X=a b c d e

.for x in $X
LIB${x:tu}=/tmp/lib$x.a
.endfor

X_LIBS= ${LIBA} ${LIBD} ${LIBE}

LIB?=a

var = head
res = no
.if !empty(var:M${:Uhead\:tail:C/:.*//})
res = OK
.endif

all:	show-libs check-cclass slow

show-libs:
	@for x in $X; do ${.MAKE} -f ${MAKEFILE} show LIB=$$x; done
	@echo "Mscanner=${res}"

show:
	@echo 'LIB=${LIB} X_LIBS:M$${LIB$${LIB:tu}} is "${X_LIBS:M${LIB${LIB:tu}}}"'
	@echo 'LIB=${LIB} X_LIBS:M*/lib$${LIB}.a is "${X_LIBS:M*/lib${LIB}.a}"'
	@echo 'LIB=${LIB} X_LIBS:M*/lib$${LIB}.a:tu is "${X_LIBS:M*/lib${LIB}.a:tu}"'

LIST= One Two Three Four five six seven

check-cclass:
	@echo Upper=${LIST:M[A-Z]*}
	@echo Lower=${LIST:M[^A-Z]*}
	@echo nose=${LIST:M[^s]*[ex]}

# Before 2020-06-13, this expression took quite a long time in Str_Match,
# calling itself 601080390 times for 16 asterisks.
slow: .PHONY
	@:;: ${:U****************:M****************b:Q}

# As of 2020-08-01, the :M and :N modifiers interpret backslashes differently,
# depending on whether there was a variable expression before the first
# backslash or not.  This can be seen by setting the -dv debug flag, in the
# lines starting with "Pattern".
#
# Apart from the different and possibly confusing debug output, there is no
# difference in behavior.  When parsing the modifier text, only \{, \} and \:
# are unescaped, and in the pattern matching these have the same meaning as
# their plain variants '{', '}' and ':'.  In the pattern matching from
# Str_Match, only \*, \? or \[ would make a noticeable difference.
SPECIALS=	\: : \\ * \*
.if ${SPECIALS:M${:U}\:} != ${SPECIALS:M\:${:U}}
.warning unexpected
.endif
