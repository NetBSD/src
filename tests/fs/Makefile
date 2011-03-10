# $NetBSD: Makefile,v 1.21 2011/03/10 20:02:26 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs

SUBDIR+= 	common .WAIT

TESTS_SUBDIRS+=	ffs hfs kernfs lfs msdosfs nfs nullfs psshfs ptyfs puffs
TESTS_SUBDIRS+=	tmpfs umapfs union
TESTS_SUBDIRS+=	vfs

.if ${MKZFS} != "no"
TESTS_SUBDIRS+=	zfs
.endif

FILES= h_funcs.subr
FILESDIR= ${TESTSDIR}

.include <bsd.test.mk>
