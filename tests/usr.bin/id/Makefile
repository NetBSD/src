# $NetBSD: Makefile,v 1.2 2019/08/15 08:22:52 kamil Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/id

TESTS_SH=	t_groups
TESTS_SH+=	t_id
TESTS_SH+=	t_whoami

BINDIR=		${TESTSDIR}
PROG=		h_id
.PATH:  	${NETBSDSRCDIR}/usr.bin/id
SRCS=		id.c pwgr.c

COPTS.id.c += -Wno-format-nonliteral

SANITIZER_RENAME_SYMBOL+=	__getpwnam50
SANITIZER_RENAME_SYMBOL+=	__getpwuid50
SANITIZER_RENAME_SYMBOL+=	getgrgid
SANITIZER_RENAME_SYMBOL+=	getgrouplist
SANITIZER_RENAME_SYMBOL+=	getgroups

.include <bsd.test.mk>
