#	$NetBSD: Makefile,v 1.15 2001/08/14 10:18:26 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	xinstall
SRCS=	stat_flags.c xinstall.c
MAN=	install.1
.PATH:	${.CURDIR}/../../bin/ls
CPPFLAGS+=	-I${.CURDIR}/../../bin/ls

PROGNAME=install

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
