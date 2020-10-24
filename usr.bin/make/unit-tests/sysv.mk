# $NetBSD: sysv.mk,v 1.15 2020/10/24 08:50:17 rillig Exp $

all: foo fun sam bla

FOO?=
FOOBAR=	${FOO:=bar}

_this:=	${.PARSEDIR}/${.PARSEFILE}

B=	/b
S=	/
FUN=	${B}${S}fun
SUN=	the Sun

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


SAM=	sam.c

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
