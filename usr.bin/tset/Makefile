#	$NetBSD: Makefile,v 1.4 1997/10/14 02:07:55 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/9/93

WARNS?=	1
PROG=	tset
SRCS=	map.c misc.c set.c term.c tset.c wrterm.c

DPADD=	${LIBTERMCAP}
LDADD=	-ltermcap
LINKS=	${BINDIR}/tset ${BINDIR}/reset
MLINKS=	tset.1 reset.1

.include <bsd.prog.mk>
