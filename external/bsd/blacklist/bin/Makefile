# $NetBSD: Makefile,v 1.11.2.2 2015/04/30 06:07:33 riz Exp $

BINDIR=/sbin

PROGS=blacklistd blacklistctl
MAN.blacklistd=blacklistd.8 blacklistd.conf.5
MAN.blacklistctl=blacklistctl.8
SRCS.blacklistd = blacklistd.c conf.c run.c state.c support.c internal.c
SRCS.blacklistctl = blacklistctl.c conf.c state.c support.c internal.c
DBG=-g

LDADD+=-lutil
DPADD+=${LIBUTIL}

.PATH: ${.CURDIR}/../port
SRCS.blacklistd+= popenve.c strtoi.c
SRCS.blacklistctl+= strtoi.c
CPPFLAGS+=-DHAVE_CONFIG_H

.include <bsd.prog.mk>
