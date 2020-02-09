#	$NetBSD: Makefile,v 1.14 2020/02/09 15:13:28 fox Exp $
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

COPTS.master.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
