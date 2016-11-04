# $NetBSD: Makefile,v 1.3.2.1 2016/11/04 14:49:23 pgoyette Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/db

TESTS_SH+=	t_db

TESTS_C+=	t_db_hash_seq

BINDIR=		${TESTSDIR}
MKMAN=		no

PROGS+=		h_db
PROGS+=		h_lfsr
CPPFLAGS.h_db.c +=	-I${NETBSDSRCDIR}/lib/libc/db/btree

FILESDIR=	${TESTSDIR}

FILES+=		README

.include <bsd.test.mk>
