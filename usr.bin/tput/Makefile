#	$NetBSD: Makefile,v 1.6 1997/10/14 01:46:32 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS?=	1
PROG=	tput
DPADD=	${LIBTERMCAP}
LDADD=	-ltermcap
MLINKS=	tput.1 clear.1
SCRIPTS=clear.sh

.include <bsd.prog.mk>
