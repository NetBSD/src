#	$NetBSD: Makefile,v 1.13 2000/12/28 22:18:27 sommerfeld Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	getty
LINKS=	${BINDIR}/getty ${BINDIR}/uugetty
SRCS=	main.c init.c subr.c
DPADD+=	${LIBUTIL} ${LIBTERMCAP}
LDADD+=	-lutil -ltermcap
MAN=	getty.8 gettytab.5 ttys.5
MLINKS+=getty.8 uugetty.8

.include <bsd.prog.mk>
