#	$NetBSD: Makefile,v 1.20 2006/12/14 20:09:36 he Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	xinstall
SRCS=	xinstall.c getid.c
MAN=	install.1

.PATH:		${NETBSDSRCDIR}/usr.sbin/mtree
CPPFLAGS+=	-I${NETBSDSRCDIR}/usr.sbin/mtree

LDADD+= -lutil
DPADD+=	${LIBUTIL}

PROGNAME=install

.include <bsd.prog.mk>
