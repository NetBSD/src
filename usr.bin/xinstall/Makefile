#	$NetBSD: Makefile,v 1.14 1998/10/10 07:38:22 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	xinstall
SRCS=	stat_flags.c xinstall.c
MAN=	install.1
.PATH:	${.CURDIR}/../../bin/ls
CPPFLAGS+=	-I${.CURDIR}/../../bin/ls

PROGNAME=install

.include <bsd.prog.mk>
