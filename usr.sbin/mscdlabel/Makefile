# $NetBSD: Makefile,v 1.1 2002/05/29 19:39:06 drochner Exp $

PROG=	mscdlabel
SRCS=	main.c dkcksum.c
LDADD=	-lutil
MAN=	mscdlabel.8
WARNS=	1

DISKLABEL_SRC=	${.CURDIR}/../../sbin/disklabel

.PATH:	${DISKLABEL_SRC}
CPPFLAGS+=	-I${DISKLABEL_SRC}

.include <bsd.prog.mk>
