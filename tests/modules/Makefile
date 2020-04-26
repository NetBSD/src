# $NetBSD: Makefile,v 1.19 2020/04/26 09:08:40 maxv Exp $

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
TESTS_C+=	t_ufetchstore
CPPFLAGS.t_ufetchstore.c+=-I${.CURDIR}/ufetchstore
.for i in t_modctl t_builtin t_kcov
LDADD.${i}=	-lprop
LDADD.${i}+=	-lrumpfs_kernfs ${LIBRUMPBASE}
.endfor

.if ${MACHINE} == "amd64"
TESTS_C+=	t_x86_pte
.endif

TESTS_SH=	t_abi_uvm
TESTS_SH+=	t_modload
TESTS_SH+= 	t_klua_pr_52864
TESTS_SH+=	t_threadpool

SUBDIR=		k_helper
SUBDIR+=	k_helper2
SUBDIR+=	k_helper3
SUBDIR+=	k_uvm
SUBDIR+=	threadpool_tester
SUBDIR+=	ufetchstore
.if ${MACHINE} == "amd64"
SUBDIR+=	x86_pte_tester
.endif

.include <bsd.test.mk>
