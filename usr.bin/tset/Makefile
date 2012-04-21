#	$NetBSD: Makefile,v 1.7 2012/04/21 12:27:30 roy Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/9/93

PROG=	tset
SRCS=	map.c misc.c set.c term.c tset.c

DPADD=	${LIBTERMINFO}
LDADD=	-lterminfo
LINKS=	${BINDIR}/tset ${BINDIR}/reset
MLINKS=	tset.1 reset.1

.include <bsd.prog.mk>
