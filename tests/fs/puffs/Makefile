# $NetBSD: Makefile,v 1.2 2007/12/26 22:33:54 jmmv Exp $

TESTSDIR=	${TESTSBASE}/fs/puffs

PROGS=			h_have_puffs
SRCS.h_have_puffs=	h_have_puffs.c
MAN.h_have_puffs=	# empty
BINDIR.h_have_puffs=	${TESTSDIR}

TESTS_SH=	t_psshfs

FILESDIR=	${TESTSDIR}
FILES=		ssh_config.in
FILES+=		ssh_host_key
FILES+=		ssh_host_key.pub
FILES+=		sshd_config.in

.include <bsd.test.mk>
