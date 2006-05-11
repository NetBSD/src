#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93
#	$NetBSD: Makefile,v 1.10 2006/05/11 07:19:33 mrg Exp $

PROG=	rbootd
SRCS=	bpf.c conf.c parseconf.c rbootd.c rmpproto.c utils.c
MAN=	rbootd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.own.mk>

.if ${HAVE_GCC} == 4
COPTS.utils.c+=	-fno-strict-aliasing
.endif

.include <bsd.prog.mk>
