# $NetBSD: Makefile,v 1.26 2025/12/24 15:37:30 thorpej Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs

SUBDIR+= 	common .WAIT

TESTS_SUBDIRS+=	ffs fifofs psshfs puffs
TESTS_SUBDIRS+=	vfs

.if ${MKCD9660FS} != "no"
TESTS_SUBDIRS+=	cd9660
.endif

.if ${MKHFS} != "no"
TESTS_SUBDIRS+=	hfs
.endif

.if ${MKKERNFS} != "no"
TESTS_SUBDIRS+=	kernfs
.endif

.if ${MKLFS} != "no"
TESTS_SUBDIRS+=	lfs
.endif

.if ${MKMSDOSFS} != "no"
TESTS_SUBDIRS+=	msdosfs
.endif

.if ${MKNFS} != "no"
TESTS_SUBDIRS+=	nfs
.endif

.if ${MKNULLFS} != "no"
TESTS_SUBDIRS+=	nullfs
.endif

.if ${MKPTYFS} != "no"
TESTS_SUBDIRS+=	ptyfs
.endif

.if ${MKTMPFS} != "no"
TESTS_SUBDIRS+=	tmpfs
.endif

.if ${MKUMAPFS} != "no"
TESTS_SUBDIRS+=	umapfs
.endif

.if ${MKUNIONFS} != "no"
TESTS_SUBDIRS+=	union
.endif

.if ${MKZFS} != "no"
TESTS_SUBDIRS+=	zfs
.endif

FILES= h_funcs.subr
FILESDIR= ${TESTSDIR}

.include <bsd.test.mk>
