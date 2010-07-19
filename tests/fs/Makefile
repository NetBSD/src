# $NetBSD: Makefile,v 1.16 2010/07/19 16:09:07 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs

TESTS_SUBDIRS+=	common .WAIT ffs kernfs lfs msdosfs nullfs psshfs ptyfs puffs
TESTS_SUBDIRS+=	tmpfs umapfs union
TESTS_SUBDIRS+=	vfs

FILES= h_funcs.subr
FILESDIR= ${TESTSDIR}

.include <bsd.test.mk>
