# $Id: sysv.mk,v 1.12 2020/08/01 13:35:13 rillig Exp $

all: foo fun sam bla words ampersand anchor-dollar
all: mismatch

FOO ?=
FOOBAR = ${FOO:=bar}

_this := ${.PARSEDIR}/${.PARSEFILE}

B = /b
S = /
FUN = ${B}${S}fun
SUN = the Sun

# we expect nothing when FOO is empty
foo:
	@echo FOOBAR = ${FOOBAR}
.if empty(FOO)
	@FOO="foo fu" ${.MAKE} -f ${_this} foo
.endif

fun:
	@echo ${FUN:T}
	@echo ${FUN:${B}${S}fun=fun}
	@echo ${FUN:${B}${S}%=%}
	@echo ${In:L:%=% ${SUN}}


SAM=sam.c

sam:
	@echo ${SAM:s%.c=acme}
	@echo ${SAM:s%.c=a%.d}
	@echo ${SAM:s.c=a%.d}
	@echo ${SAM:sam.c=a%.c}
	@echo ${SAM:%=a%.c}
	@echo ${SAM:%.c=a%.c}
	@echo ${SAM:sam%=a%.c}

BLA=

bla:
	@echo $(BLA:%=foo/%x)

# The :Q looks like a modifier but isn't.
# It is part of the replacement string.
words:
	@echo a${a b c d e:L:%a=x:Q}b

# Before 2020-07-19, an ampersand could be used in the replacement part
# of a SysV substitution modifier.  This was probably a copy-and-paste
# mistake since the SysV modifier code looked a lot like the code for the
# :S and :C modifiers.  The ampersand is not mentioned in the manual page.
ampersand:
	@echo ${:U${a.bcd.e:L:a.%=%}:Q}
	@echo ${:U${a.bcd.e:L:a.%=&}:Q}

# Before 2020-07-20, when a SysV modifier was parsed, a single dollar
# before the '=' was interpreted as an anchor, which doesn't make sense
# since the anchor was discarded immediately.
anchor-dollar:
	@echo $@: ${:U${value:L:e$=x}:Q}
	@echo $@: ${:U${value:L:e=x}:Q}

# Words that don't match are copied unmodified.
# The % placeholder can be anywhere in the string.
mismatch:
	@echo $@: ${:Ufile.c file.h:%.c=%.cpp}
	@echo $@: ${:Ufile.c other.c:file.%=renamed.%}

# Trying to cover all possible variants of the SysV modifier.
LIST=	one two
EXPR.1=	${LIST:o=X}
EXP.1=	one twX
EXPR.2=	${LIST:o=}
EXP.2=	one tw
EXPR.3=	${LIST:o=%}
EXP.3=	one tw%
EXPR.4=	${LIST:%o=X}
EXP.4=	one X
EXPR.5=	${LIST:o%=X}
EXP.5=	X two
EXPR.6=	${LIST:o%e=X}
EXP.6=	X two
EXPR.7=	${LIST:o%%e=X}		# Only the first '%' is the wildcard.
EXP.7=	one two			# None of the words contains a literal '%'.
EXPR.8=	${LIST:%=%%}
EXP.8=	one% two%
EXPR.9=	${LIST:%nes=%xxx}	# lhs is longer than the word "one"
EXP.9=	one two

.for i in ${:U:range=9}
.if ${EXPR.$i} != ${EXP.$i}
.warning test case $i expected "${EXP.$i}", got "${EXPR.$i}
.endif
.endfor
