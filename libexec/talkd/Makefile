#	$NetBSD: Makefile,v 1.9 2002/08/20 13:56:50 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	ntalkd
SRCS=	talkd.c announce.c process.c table.c print.c utmpentry.c
MAN=	talkd.8
DPADD+=${LIBUTIL}
LDADD+=-lutil

.PATH.c: ${.CURDIR}/../../usr.bin/who
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
