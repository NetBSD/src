#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93
#	$NetBSD: Makefile,v 1.8 2000/04/13 08:52:44 itojun Exp $

PROG=	rbootd
SRCS=	bpf.c conf.c parseconf.c rbootd.c rmpproto.c utils.c
MAN=	rbootd.8

CPPFLAGS+=-DHAVE_IFADDRS_H

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
