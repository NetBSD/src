# $NetBSD: Makefile.inc,v 1.2.24.1 2017/03/20 06:57:54 pgoyette Exp $
WARNS ?= 4
CWARNFLAGS+= -Wno-missing-noreturn
TOOLSSRCDIR:= ${.PARSEDIR}
CPPFLAGS+= -I${TOOLSSRCDIR}
