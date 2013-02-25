# $NetBSD: Makefile,v 1.1.4.1 2013/02/25 00:30:26 tls Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.sbin/mtree

TESTS_SH=	t_mtree

FILESDIR=	${TESTSDIR}

FILES+=		d_convert.in
FILES+=		d_convert_C.out
FILES+=		d_convert_C_S.out
FILES+=		d_convert_D.out
FILES+=		d_convert_D_S.out
FILES+=		netbsd6_d_create.out
FILES+=		mtree_d_create.out
FILES+=		d_merge.in
FILES+=		d_merge_C_M.out
FILES+=		d_merge_C_M_S.out

.include <bsd.test.mk>
