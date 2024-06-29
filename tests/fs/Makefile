# $NetBSD: Makefile,v 1.25.4.1 2024/06/29 19:43:26 perseant Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs

SUBDIR+= 	common .WAIT

TESTS_SUBDIRS+=	ffs fifofs hfs kernfs lfs msdosfs nfs nullfs psshfs ptyfs puffs
TESTS_SUBDIRS+=	tmpfs umapfs union
TESTS_SUBDIRS+=	vfs cd9660

.if ${MKZFS} != "no"
TESTS_SUBDIRS+=	zfs
.endif

.if ${MKEXFATFS} != "no"
TESTS_SUBDIRS+=	exfatfs
.endif

FILES= h_funcs.subr
FILESDIR= ${TESTSDIR}

.include <bsd.test.mk>
