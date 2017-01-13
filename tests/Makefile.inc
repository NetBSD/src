# $NetBSD: Makefile.inc,v 1.3 2017/01/13 21:30:39 christos Exp $
WARNS ?= 4
CWARNFLAGS+= -Wno-missing-noreturn
CPPFLAGS:= ${CPPFLAGS} -I${.PARSEDIR}
