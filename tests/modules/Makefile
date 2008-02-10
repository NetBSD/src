# $NetBSD: Makefile,v 1.1 2008/02/10 12:40:10 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/modules

# Ideally this test could be in the parent Makefile, which could not descend
# into this directory at all.  Unfortunately, the etc/mtree/NetBSD.dist file
# creates the 'modules' subdirectory unconditionally, which if left empty
# will confuse atf-run.  Therefore we must install, at the very least, the
# Atffile into it.
.if ${MKMODULAR} != no

TESTS_CXX=	t_modctl

SUBDIR=		k_helper

.endif

.include <bsd.subdir.mk>
.include <bsd.test.mk>
