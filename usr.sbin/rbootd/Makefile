#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93
#	$NetBSD: Makefile,v 1.9 2003/05/15 14:50:50 itojun Exp $

PROG=	rbootd
SRCS=	bpf.c conf.c parseconf.c rbootd.c rmpproto.c utils.c
MAN=	rbootd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
