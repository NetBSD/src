#	$NetBSD: Makefile,v 1.15 2023/06/03 09:09:24 lukem Exp $
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

CWARNFLAGS.clang+=	-Wno-error=implicit-int-float-conversion

COPTS.master.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
