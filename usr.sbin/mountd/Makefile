#	$NetBSD: Makefile,v 1.16 2002/09/21 20:35:00 christos Exp $
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
