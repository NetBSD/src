# $NetBSD: Makefile,v 1.1 2012/03/17 16:33:16 jruoho Exp $

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
FILES+=		d_create.out
FILES+=		d_merge.in
FILES+=		d_merge_C_M.out
FILES+=		d_merge_C_M_S.out

.include <bsd.test.mk>
