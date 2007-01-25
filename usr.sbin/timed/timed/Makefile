#	$NetBSD: Makefile,v 1.12 2007/01/25 23:51:11 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#
# optional flags are: MEASURE TESTING DEBUG

PROG=	timed
SRCS=	acksend.c candidate.c correct.c master.c networkdelta.c readmsg.c \
	slave.c timed.c byteorder.c measure.c cksum.c tspname.c
DPADD=	${LIBUTIL}
LDADD=	-lutil
CPPFLAGS+=-DSUPPORT_UTMP -DSUPPORT_UTMPX
MAN=	timed.8

.include <bsd.prog.mk>
