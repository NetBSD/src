# $NetBSD: Makefile,v 1.24 2022/05/27 07:28:21 nia Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs

SUBDIR+= 	common .WAIT

TESTS_SUBDIRS+=	ffs fifofs hfs kernfs msdosfs nfs nullfs psshfs ptyfs puffs
TESTS_SUBDIRS+=	tmpfs umapfs union
TESTS_SUBDIRS+=	vfs cd9660

.if ${MKLFS} != "no"
TESTS_SUBDIRS+=	lfs
.endif

.if ${MKZFS} != "no"
TESTS_SUBDIRS+=	zfs
.endif

FILES= h_funcs.subr
FILESDIR= ${TESTSDIR}

.include <bsd.test.mk>
