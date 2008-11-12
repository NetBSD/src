# $NetBSD: Makefile,v 1.4 2008/11/12 12:36:29 ad Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/modules

# Ideally this test could be in the parent Makefile, which could not descend
# into this directory at all.  Unfortunately, the etc/mtree/NetBSD.dist file
# creates the 'modules' subdirectory unconditionally, which if left empty
# will confuse atf-run.  Therefore we must install, at the very least, the
# Atffile into it.
TESTS_C=	t_modctl
LDADD=		-lprop

TESTS_SH=	t_modload

SUBDIR=		k_helper

.include <bsd.subdir.mk>
.include <bsd.test.mk>
