#	$NetBSD: Makefile,v 1.17 2003/05/16 14:53:55 itojun Exp $
#	@(#)Makefile	8.3 (Berkeley) 1/25/94

PROG=	mountd
SRCS=	mountd.c ipsec.c
MAN=	exports.5 mountd.8

INETDDIR= ${.CURDIR}/../inetd

CPPFLAGS+= -I${INETDDIR} -DIPSEC
LDADD+=	-lipsec -lutil
DPADD+=	${LIBIPSEC} ${LIBUTIL}

.include <bsd.prog.mk>

.PATH: ${INETDDIR}
