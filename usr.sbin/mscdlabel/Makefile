# $NetBSD: Makefile,v 1.3 2002/11/30 06:00:42 lukem Exp $

.include <bsd.own.mk>

PROG=	mscdlabel
SRCS=	main.c dkcksum.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
MAN=	mscdlabel.8

DISKLABEL_SRC=	${NETBSDSRCDIR}/sbin/disklabel

.PATH:	${DISKLABEL_SRC}
CPPFLAGS+=	-I${DISKLABEL_SRC}

.include <bsd.prog.mk>
