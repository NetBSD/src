#	$NetBSD: Makefile,v 1.10.12.1 2013/02/25 00:30:40 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	tput
DPADD=	${LIBTERMINFO}
LDADD=	-lterminfo
MLINKS=	tput.1 clear.1
SCRIPTS=clear.sh

CPPFLAGS+=	-I${.CURDIR}/../../lib/libterminfo

.include <bsd.prog.mk>
