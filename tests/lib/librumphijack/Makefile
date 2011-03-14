#	$NetBSD: Makefile,v 1.7 2011/03/14 15:57:23 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/librumphijack

TESTS_SH=	t_asyncio
TESTS_SH+=	t_config
TESTS_SH+=	t_cwd
TESTS_SH+=	t_sh
TESTS_SH+=	t_tcpip
TESTS_SH+=	t_vfs
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
