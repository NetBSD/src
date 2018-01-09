#	$NetBSD: Makefile,v 1.1 2018/01/09 03:31:15 christos Exp $

WARNS=6
PROG=	automount
SRCS=	automount.c automountd.c autounmountd.c common.c \
	defined.c log.c popen.c token.l

CFLAGS+=	-I${.CURDIR} -I${.CURDIR}/../../sys

MAN=	automount.8 automountd.8 autounmountd.8 auto_master.5

DPADD=	${LIBUTIL}
LDADD=	-lutil

LINKS=	${BINDIR}/automount ${BINDIR}/automountd
LINKS+=	${BINDIR}/automount ${BINDIR}/autounmountd

.include <bsd.prog.mk>
