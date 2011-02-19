#	$NetBSD: Makefile,v 1.4 2011/02/19 13:19:52 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/librumphijack

TESTS_SH=	t_asyncio
TESTS_SH+=	t_cwd
TESTS_SH+=	t_tcpip
TESTS_C=	h_client
TESTS_C+=	h_netget
TESTS_C+=	h_cwd

FILES=		netstat.expout index.html
FILESDIR=	${TESTSDIR}

# ssh test
FILES+=		ssh_config.in
FILES+=		ssh_host_key
FILES+=		ssh_host_key.pub
FILES+=		sshd_config.in

ATFFILE=	yes

.include <bsd.test.mk>
