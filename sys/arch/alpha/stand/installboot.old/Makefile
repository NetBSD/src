# $NetBSD: Makefile,v 1.12 1999/04/05 05:07:04 cgd Exp $

PROG=	installboot.old

.PATH:	${.CURDIR}/../../../../../sbin/disklabel
SRCS=	installboot.c cd9660.c dkcksum.c

LDADD+=	-lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
