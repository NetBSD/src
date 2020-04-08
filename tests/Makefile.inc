# $NetBSD: Makefile.inc,v 1.5.14.1 2020/04/08 14:09:05 martin Exp $
WARNS ?= 4
CWARNFLAGS+= -Wno-missing-noreturn
TOOLSSRCDIR:= ${.PARSEDIR}
CPPFLAGS+= -I${TOOLSSRCDIR}
LIBRUMPBASE = -lrumpvfs -lrump -lrumpvfs -lrumpuser -lrump -lpthread

