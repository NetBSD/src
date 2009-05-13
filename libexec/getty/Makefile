#	$NetBSD: Makefile,v 1.17.26.1 2009/05/13 19:18:38 jym Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

WARNS?=	2	# XXX: many const & sign-compare issues

PROG=	getty
SRCS=	main.c init.c subr.c
DPADD+=	${LIBUTIL} ${LIBTERMCAP}
LDADD+=	-lutil -ltermcap
MAN=	getty.8 gettytab.5 ttys.5

.include <bsd.prog.mk>
