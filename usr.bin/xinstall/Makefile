#	$NetBSD: Makefile,v 1.17 2002/09/18 14:00:44 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	xinstall
SRCS=	${HOSTPROG:D:Ustat_flags.c} xinstall.c
MAN=	install.1
.PATH:	${NETBSDSRCDIR}/bin/ls
CPPFLAGS+=	-I${NETBSDSRCDIR}/bin/ls

PROGNAME=install

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
