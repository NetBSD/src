#	$NetBSD: Makefile,v 1.8 1997/10/08 09:07:37 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	ntalkd
SRCS=	talkd.c announce.c process.c table.c print.c
MAN=	talkd.8
DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
