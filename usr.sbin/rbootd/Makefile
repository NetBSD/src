#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93
#	$NetBSD: Makefile,v 1.14.32.1 2020/04/13 08:05:57 martin Exp $

USE_FORT?= yes	# network server

PROG=	rbootd
SRCS=	bpf.c conf.c parseconf.c rbootd.c rmpproto.c utils.c
MAN=	rbootd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.own.mk>

.if defined(HAVE_GCC) || defined(HAVE_LLVM)
COPTS.utils.c+=	-fno-strict-aliasing
.endif

COPTS.bpf.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
