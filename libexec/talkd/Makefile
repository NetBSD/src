#	$NetBSD: Makefile,v 1.7 1997/07/06 22:52:28 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

WARNS=	1
PROG=	ntalkd
SRCS=	talkd.c announce.c process.c table.c print.c
MAN=	talkd.8
DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
