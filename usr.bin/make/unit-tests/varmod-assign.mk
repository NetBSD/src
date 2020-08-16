# $NetBSD: varmod-assign.mk,v 1.2 2020/08/16 12:48:55 rillig Exp $
#
# Tests for the obscure ::= variable modifiers, which perform variable
# assignments during evaluation, just like the = operator in C.

all:	mod-assign
all:	mod-assign-nested

mod-assign:
	@echo $@: ${1 2 3:L:@i@${FIRST::?=$i}@} first=${FIRST}.
	@echo $@: ${1 2 3:L:@i@${LAST::=$i}@} last=${LAST}.
	@echo $@: ${1 2 3:L:@i@${APPENDED::+=$i}@} appended=${APPENDED}.
	@echo $@: ${echo.1 echo.2 echo.3:L:@i@${RAN::!=${i:C,.*,&; & 1>\&2,:S,., ,g}}@} ran:${RAN}.
	# The assignments happen in the global scope and thus are
	# preserved even after the shell command has been run.
	@echo $@: global: ${FIRST:Q}, ${LAST:Q}, ${APPENDED:Q}, ${RAN:Q}.

mod-assign-nested:
	@echo $@: ${1:?${THEN1::=then1${IT1::=t1}}:${ELSE1::=else1${IE1::=e1}}}${THEN1}${ELSE1}${IT1}${IE1}
	@echo $@: ${0:?${THEN2::=then2${IT2::=t2}}:${ELSE2::=else2${IE2::=e2}}}${THEN2}${ELSE2}${IT2}${IE2}
	@echo $@: ${SINK3:Q}
	@echo $@: ${SINK4:Q}
SINK3:=	${1:?${THEN3::=then3${IT3::=t3}}:${ELSE3::=else3${IE3::=e3}}}${THEN3}${ELSE3}${IT3}${IE3}
SINK4:=	${0:?${THEN4::=then4${IT4::=t4}}:${ELSE4::=else4${IE4::=e4}}}${THEN4}${ELSE4}${IT4}${IE4}
