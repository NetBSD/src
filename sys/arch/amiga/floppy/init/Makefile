#	$NetBSD: Makefile,v 1.2 1994/10/26 02:05:33 cgd Exp $

#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	BSDI Id: Makefile,v 1.3 1993/06/14 20:06:22 donn Exp 

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
