#	$NetBSD: Makefile,v 1.8 2005/07/30 14:43:13 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS=	3
PROG=	tput
DPADD=	${LIBTERMCAP}
LDADD=	-ltermcap
MLINKS=	tput.1 clear.1
SCRIPTS=clear.sh

.include <bsd.prog.mk>
