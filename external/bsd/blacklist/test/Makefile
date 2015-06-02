# $NetBSD: Makefile,v 1.2.2.3 2015/06/02 20:32:44 snj Exp $

MKMAN=no

PROGS=srvtest cltest 
SRCS.srvtest = srvtest.c
SRCS.cltest = cltest.c
CPPFLAGS+=-DBLDEBUG
LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
