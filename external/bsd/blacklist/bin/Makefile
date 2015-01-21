# $NetBSD: Makefile,v 1.5 2015/01/21 16:16:00 christos Exp $

BINDIR=/usr/sbin

PROGS=blacklistd blacklistctl
MAN=blacklistd.8
MLINKS=blacklistd.8 blacklistd.conf.5
SRCS.blacklistd = blacklistd.c conf.c run.c state.c
SRCS.blacklistctl = blacklistctl.c conf.c state.c

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
