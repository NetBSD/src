#	$NetBSD: Makefile,v 1.4 2002/03/05 04:11:51 itojun Exp $

.include <bsd.own.mk>

PROG=	altqd
MAN=	altqd.8 altq.conf.5

SRCS=	altqd.c libaltq2.c

CPPFLAGS+= -DALTQ -I${.CURDIR}/../libaltq

LIBALTQOBJ != cd ${.CURDIR}/../libaltq && ${PRINTOBJDIR}
DPADD+=	${LIBALTQOBJ}/libaltq.a
LDADD+=	-L${LIBALTQOBJ} -laltq

DPADD+=	${LIBUTIL} ${LIBM}
LDADD+=	-lutil -lm

.include <bsd.prog.mk>
