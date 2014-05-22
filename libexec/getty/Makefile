#	$NetBSD: Makefile,v 1.19.6.1 2014/05/22 11:37:13 yamt Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	getty
SRCS=	main.c init.c subr.c
DPADD+=	${LIBUTIL} ${LIBTERMINFO}
LDADD+=	-lutil -lterminfo
MAN=	getty.8 gettytab.5 ttys.5

.include <bsd.prog.mk>
