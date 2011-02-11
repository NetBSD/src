#	$NetBSD: Makefile,v 1.2 2011/02/11 15:38:14 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/librumphijack

TESTS_SH=	t_asyncio
TESTS_SH+=	t_tcpip
TESTS_C=	h_client
TESTS_C+=	h_netget

FILES=		netstat.expout index.html
FILESDIR=	${TESTSDIR}

ATFFILE=	yes

.include <bsd.test.mk>
