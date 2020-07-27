# $Id: export.mk,v 1.2 2020/07/27 19:45:56 rillig Exp $

UT_TEST=export
UT_FOO=foo${BAR}
UT_FU=fubar
UT_ZOO=hoopie
UT_NO=all
# belive it or not, we expect this one to come out with $UT_FU unexpanded.
UT_DOLLAR= This is $$UT_FU

.export UT_FU UT_FOO
.export UT_DOLLAR
# this one will be ignored
.export .MAKE.PID

BAR=bar is ${UT_FU}

.MAKE.EXPORTED+= UT_ZOO UT_TEST

FILTER_CMD?=	grep -v -E '^(MAKEFLAGS|PATH|PWD)='

all:
	@env | ${FILTER_CMD} | sort
