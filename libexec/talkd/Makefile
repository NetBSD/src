#	$NetBSD: Makefile,v 1.12 2004/03/24 21:20:26 he Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

PROG=	ntalkd
SRCS=	talkd.c announce.c process.c table.c print.c utmpentry.c
MAN=	talkd.8
MLINKS+=talkd.8 ntalkd.8
DPADD+=${LIBUTIL}
LDADD+=-lutil

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
