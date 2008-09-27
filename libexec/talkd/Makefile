#	$NetBSD: Makefile,v 1.13 2008/09/27 04:41:20 reed Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

PROG=	ntalkd
SRCS=	talkd.c announce.c process.c table.c print.c utmpentry.c
MAN=	ntalkd.8
BUILDSYMLINKS+= talkd.8 ntalkd.8
MLINKS+=ntalkd.8 talkd.8
DPADD+=${LIBUTIL}
LDADD+=-lutil

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
