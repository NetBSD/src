#	$NetBSD: Makefile,v 1.11 1997/10/19 23:46:07 cjs Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	getty
LINKS=	${BINDIR}/getty ${BINDIR}/uugetty
SRCS=	main.c init.c subr.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
MAN=	getty.8 gettytab.5 ttys.5

.include <bsd.prog.mk>
