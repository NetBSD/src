# $NetBSD: Makefile,v 1.15 2019/02/23 03:10:06 kamil Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/modules
CPPFLAGS+=	-D_KERNTYPES

# Ideally this test could be in the parent Makefile, which could not descend
# into this directory at all.  Unfortunately, the etc/mtree/NetBSD.dist file
# creates the 'modules' subdirectory unconditionally, which if left empty
# will confuse atf-run.  Therefore we must install, at the very least, the
# Atffile into it.
TESTS_C=	t_modctl
TESTS_C+=	t_builtin
TESTS_C+=	t_kcov
LDADD=		-lprop
LDADD+=		-lrumpfs_kernfs -lrumpvfs -lrump -lrumpuser -lrump -lpthread

TESTS_SH=	t_abi_uvm
TESTS_SH+=	t_modload
TESTS_SH+= 	t_klua_pr_52864
TESTS_SH+=	t_threadpool

SUBDIR=		k_helper
SUBDIR+=	k_helper2
SUBDIR+=	k_helper3
SUBDIR+=	k_uvm
SUBDIR+=	threadpool_tester

.include <bsd.test.mk>
