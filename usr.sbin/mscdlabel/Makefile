# $NetBSD: Makefile,v 1.2 2002/09/18 03:54:33 lukem Exp $

.include <bsd.own.mk>

PROG=	mscdlabel
SRCS=	main.c dkcksum.c
LDADD=	-lutil
MAN=	mscdlabel.8
WARNS=	1

DISKLABEL_SRC=	${NETBSDSRCDIR}/sbin/disklabel

.PATH:	${DISKLABEL_SRC}
CPPFLAGS+=	-I${DISKLABEL_SRC}

.include <bsd.prog.mk>
