# $NetBSD: Makefile,v 1.18 2010/07/26 15:53:00 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs

SUBDIR+= 	common .WAIT nfs

TESTS_SUBDIRS+=	ffs kernfs lfs msdosfs nullfs psshfs ptyfs puffs
TESTS_SUBDIRS+=	tmpfs umapfs union
TESTS_SUBDIRS+=	vfs

FILES= h_funcs.subr
FILESDIR= ${TESTSDIR}

.include <bsd.test.mk>
