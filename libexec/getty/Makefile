#	$NetBSD: Makefile,v 1.17 2006/11/16 04:43:32 atatat Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	getty
SRCS=	main.c init.c subr.c
DPADD+=	${LIBUTIL} ${LIBTERMCAP}
LDADD+=	-lutil -ltermcap
MAN=	getty.8 gettytab.5 ttys.5

.include <bsd.prog.mk>
