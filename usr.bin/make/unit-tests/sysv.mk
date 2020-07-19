# $Id: sysv.mk,v 1.7 2020/07/19 22:04:27 rillig Exp $

all: foo fun sam bla words ampersand

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
