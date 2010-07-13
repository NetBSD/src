# $NetBSD: Makefile,v 1.15 2010/07/13 21:13:22 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs

TESTS_SUBDIRS+=	ffs kernfs lfs msdosfs nullfs psshfs ptyfs puffs
TESTS_SUBDIRS+=	tmpfs umapfs union
TESTS_SUBDIRS+=	vfs

FILES= h_funcs.subr
FILESDIR= ${TESTSDIR}

.include <bsd.test.mk>
