#	$NetBSD: Makefile,v 1.16 2002/01/21 20:00:02 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	xinstall
SRCS=	${HOSTPROG:D:Ustat_flags.c} xinstall.c
MAN=	install.1
.PATH:	${.CURDIR}/../../bin/ls
CPPFLAGS+=	-I${.CURDIR}/../../bin/ls

PROGNAME=install

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
