#	$NetBSD: Makefile,v 1.3 1994/10/26 02:32:02 cgd Exp $

PROG=	init
SRCS=	init.c
NOMAN=
CFLAGS+=-DNOSYSCTL -DLETS_GET_SMALL
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
BINMODE=500

BINDIR=	/sbin

.PATH:	${.CURDIR}/../../../../../sbin/init

.include <bsd.prog.mk>
