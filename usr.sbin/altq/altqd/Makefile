#	$NetBSD: Makefile,v 1.3 2001/01/11 13:15:06 enami Exp $

.include <bsd.own.mk>

PROG=	altqd
MAN=	altqd.8 altq.conf.5

SRCS=	altqd.c libaltq2.c

CPPFLAGS+= -DALTQ -I${.CURDIR}/../libaltq

LIBALTQOBJ != cd ${.CURDIR}/../libaltq && ${PRINTOBJDIR}
DPADD+=	${LIBALTQOBJ}/libaltq.a
LDADD+=	-L${LIBALTQOBJ} -laltq

DPADD+=	${LIBM}
LDADD+=	-lm

.include <bsd.prog.mk>
