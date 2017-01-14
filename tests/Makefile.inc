# $NetBSD: Makefile.inc,v 1.4 2017/01/14 00:33:19 christos Exp $
WARNS ?= 4
CWARNFLAGS+= -Wno-missing-noreturn
TOPSRCDIR:= ${.PARSEDIR}
CPPFLAGS+= -I${TOPSRCDIR}
