#	from: @(#)Makefile	5.14 (Berkeley) 12/22/91
#	$Id: Makefile,v 1.2 1993/07/30 23:50:07 mycroft Exp $

PROG=	tset
SRCS=	map.c misc.c set.c term.c tset.c wrterm.c

DPADD=	${LIBTERMCAP}
LDADD=	-ltermcap
LINKS=	${BINDIR}/tset ${BINDIR}/reset
MLINKS=	tset.1 reset.1

.include <bsd.prog.mk>
