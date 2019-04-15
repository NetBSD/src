# $NetBSD: Makefile,v 1.2 2019/04/15 20:21:35 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/modules/ufetchstore
CPPFLAGS+=	-D_KERNTYPES -I${.CURDIR}/module

# Ideally this test could be in the parent Makefile, which could not descend
# into this directory at all.  Unfortunately, the etc/mtree/NetBSD.dist file
# creates the 'modules' subdirectory unconditionally, which if left empty
# will confuse atf-run.  Therefore we must install, at the very least, the
# Atffile into it.
TESTS_C=	t_ufetchstore

SUBDIR+=	module

.include <bsd.test.mk>
