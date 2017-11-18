# $Id: varcmd.mk,v 1.2 2017/11/18 22:34:04 sjg Exp $
#
# Test behaviour of recursive make and vars set on command line.

FU=fu
FOO?=foo
.if !empty(.TARGETS)
TAG=${.TARGETS}
.endif
TAG?=default

all:	one

show:
	@echo "${TAG} FU=<v>${FU}</v> FOO=<v>${FOO}</v> VAR=<v>${VAR}</v>"

one:	show
	@${.MAKE} -f ${MAKEFILE} FU=bar FOO=goo two

two:	show
	@${.MAKE} -f ${MAKEFILE} three

three:	show
	@${.MAKE} -f ${MAKEFILE} four


.ifmake two
# this should not work
FU+= oops
_FU:= ${FU}
two: immutable
immutable:
	@echo "$@ FU='${_FU}'"
.endif
.ifmake four
VAR=Internal
.MAKEOVERRIDES+= VAR
.endif

four:	show
	@${.MAKE} -f ${MAKEFILE} five

M = x
V.y = is y
V.x = is x
V := ${V.$M}
K := ${V}

show-v:
	@echo '${TAG} v=${V} k=${K}'

five:	show show-v
	@${.MAKE} -f ${MAKEFILE} M=y six

six:	show-v
	@${.MAKE} -f ${MAKEFILE} V=override show-v

