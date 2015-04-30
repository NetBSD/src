# $NetBSD: Makefile,v 1.2.2.2 2015/04/30 06:07:34 riz Exp $

MKMAN=no

PROGS=srvtest cltest 
SRCS.srvtest = srvtest.c
SRCS.cltest = cltest.c
LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
