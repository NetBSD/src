# $NetBSD: Makefile,v 1.6 2015/01/21 19:23:42 christos Exp $

BINDIR=/usr/sbin

PROGS=blacklistd blacklistctl
MAN.blacklistd=blacklistd.8
MAN.blacklistctl=blacklistctl.1
MLINKS=blacklistd.8 blacklistd.conf.5
SRCS.blacklistd = blacklistd.c conf.c run.c state.c util.c
SRCS.blacklistctl = blacklistctl.c conf.c state.c util.c
DBG=-g

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
