#	$NetBSD: Makefile,v 1.9 1998/01/05 05:59:45 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#
# optional flags are: MEASURE TESTING DEBUG

PROG=	timed
SRCS=	acksend.c candidate.c correct.c master.c networkdelta.c readmsg.c \
	slave.c timed.c byteorder.c measure.c cksum.c
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN=	timed.8

.include <bsd.prog.mk>
