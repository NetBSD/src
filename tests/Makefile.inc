# $NetBSD: Makefile.inc,v 1.6 2020/03/01 18:08:12 christos Exp $
WARNS ?= 4
CWARNFLAGS+= -Wno-missing-noreturn
TOOLSSRCDIR:= ${.PARSEDIR}
CPPFLAGS+= -I${TOOLSSRCDIR}
LIBRUMPBASE = -lrumpvfs -lrump -lrumpvfs -lrumpuser -lrump -lpthread

