#	$NetBSD: Makefile,v 1.8 1996/07/31 20:40:24 thorpej Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	getty
SRCS=	main.c init.c subr.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
MAN=	getty.8 gettytab.5 ttys.5

.include <bsd.prog.mk>
