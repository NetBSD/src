#	$NetBSD: Makefile,v 1.2 2004/11/14 11:26:48 yamt Exp $
#	$OpenBSD: Makefile,v 1.12 2004/04/25 19:24:52 deraadt Exp $

PROG=	authpf
MAN=	authpf.8
BINOWN= root
BINGRP= authpf
BINMODE= 6555
SRCS=	authpf.c

LDADD+= -lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/usr.sbin/authpf
