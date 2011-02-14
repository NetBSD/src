#	$NetBSD: Makefile,v 1.3 2011/02/14 15:14:00 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/librumphijack

TESTS_SH=	t_asyncio
TESTS_SH+=	t_tcpip
TESTS_C=	h_client
TESTS_C+=	h_netget

FILES=		netstat.expout index.html
FILESDIR=	${TESTSDIR}

# ssh test
FILES+=		ssh_config.in
FILES+=		ssh_host_key
FILES+=		ssh_host_key.pub
FILES+=		sshd_config.in

ATFFILE=	yes

.include <bsd.test.mk>
