#	$NetBSD: Makefile,v 1.9 2009/04/14 22:15:27 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	tput
DPADD=	${LIBTERMCAP}
LDADD=	-ltermcap
MLINKS=	tput.1 clear.1
SCRIPTS=clear.sh

.include <bsd.prog.mk>
