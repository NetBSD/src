#	$NetBSD: Makefile,v 1.6 2010/02/03 15:34:46 roy Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/9/93

PROG=	tset
SRCS=	map.c misc.c set.c term.c tset.c wrterm.c

DPADD=	${LIBTERMINFO}
LDADD=	-lterminfo
LINKS=	${BINDIR}/tset ${BINDIR}/reset
MLINKS=	tset.1 reset.1

.include <bsd.prog.mk>
