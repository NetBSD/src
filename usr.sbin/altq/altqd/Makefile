#	$NetBSD: Makefile,v 1.2 2000/12/16 18:57:34 thorpej Exp $

PROG=	altqd
MAN=	altqd.8 altq.conf.5

SRCS=	altqd.c libaltq2.c

CPPFLAGS+= -DALTQ -I${.CURDIR}/../libaltq

LIBALTQOBJ != cd ${.CURDIR}/../libaltq && make print-objdir
DPADD+=	${LIBALTQOBJ}/libaltq.a
LDADD+=	-L${LIBALTQOBJ} -laltq

DPADD+=	${LIBM}
LDADD+=	-lm

.include <bsd.prog.mk>
