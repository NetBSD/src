#	$NetBSD: Makefile,v 1.10.6.1 2014/05/22 11:42:50 yamt Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	tput
DPADD=	${LIBTERMINFO}
LDADD=	-lterminfo
MLINKS=	tput.1 clear.1
SCRIPTS=clear.sh

CPPFLAGS+=	-I${.CURDIR}/../../lib/libterminfo

.include <bsd.prog.mk>
