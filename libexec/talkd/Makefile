#	$NetBSD: Makefile,v 1.6 1997/06/29 19:22:04 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

CFLAGS+=-Wall -Wstrict-prototypes -Wmissing-prototypes
PROG=	ntalkd
SRCS=	talkd.c announce.c process.c table.c print.c
MAN=	talkd.8
DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
