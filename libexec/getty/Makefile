#	$NetBSD: Makefile,v 1.12 1998/03/28 06:10:32 cjs Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	getty
LINKS=	${BINDIR}/getty ${BINDIR}/uugetty
SRCS=	main.c init.c subr.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
MAN=	getty.8 gettytab.5 ttys.5
MLINKS+=getty.8 uugetty.8

.include <bsd.prog.mk>
