# $NetBSD: Makefile,v 1.8 2015/01/21 23:26:26 christos Exp $

BINDIR=/usr/sbin

PROGS=blacklistd blacklistctl
MAN.blacklistd=blacklistd.8
MAN.blacklistctl=blacklistctl.8
MLINKS=blacklistd.8 blacklistd.conf.5
SRCS.blacklistd = blacklistd.c conf.c run.c state.c util.c internal.c
SRCS.blacklistctl = blacklistctl.c conf.c state.c util.c internal.c
DBG=-g

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
