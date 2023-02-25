# $NetBSD: varname-dot-makeoverrides.mk,v 1.4 2023/02/25 00:09:52 rillig Exp $
#
# Tests for the special .MAKEOVERRIDES variable.

all:
	@${MAKE} -r -f ${MAKEFILE} dollars_stage_1

# Demonstrate that '$' characters are altered when they are passed on to child
# make processes via .MAKEOVERRIDES and MAKEFLAGS.
dollars_stage_1:
	${MAKE} -r -f ${MAKEFILE} dollars_stage_2 DOLLARS='$$$${varname}'

dollars_stage_2:
	@echo 'stage 2: dollars=<${DOLLARS}>'
	${MAKE} -r -f ${MAKEFILE} dollars_stage_3

dollars_stage_3:
	@echo 'stage 3: dollars=<${DOLLARS}>'
