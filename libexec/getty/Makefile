#	$NetBSD: Makefile,v 1.9 1997/08/06 07:23:34 mikel Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	getty
SRCS=	main.c init.c subr.c
WARNS?=	1
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
MAN=	getty.8 gettytab.5 ttys.5

.include <bsd.prog.mk>
