# $NetBSD: Makefile,v 1.19.2.1 2011/03/05 15:10:53 bouyer Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs

SUBDIR+= 	common .WAIT

TESTS_SUBDIRS+=	ffs hfs kernfs lfs msdosfs nfs nullfs psshfs ptyfs puffs
TESTS_SUBDIRS+=	tmpfs umapfs union
TESTS_SUBDIRS+=	vfs

FILES= h_funcs.subr
FILESDIR= ${TESTSDIR}

.include <bsd.test.mk>
