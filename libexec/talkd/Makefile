#	$NetBSD: Makefile,v 1.10 2002/09/19 03:17:50 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

PROG=	ntalkd
SRCS=	talkd.c announce.c process.c table.c print.c utmpentry.c
MAN=	talkd.8
DPADD+=${LIBUTIL}
LDADD+=-lutil

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
