#	$NetBSD: Makefile,v 1.3 2003/02/03 07:45:05 perseant Exp $

NOMAN=	# defined

TMPMP= /tmp/lfsregresstest_mount
TMPIM= /var/tmp/lfsregresstest.im
SRCDIR?= ${.CURDIR}

VND?= vnd0
MPART?= a
.if (${MACHINE_ARCH} == "i386")
RPART?= d
.else
RPART?= c
.endif
BVND= /dev/${VND}
CVND= /dev/r${VND}

CLEAN= clean

regress: test_mkfs_mount .WAIT test_smallfiles .WAIT test_rename_hardlink .WAIT ${CLEAN}

#
# Basic regression tests for LFS.
#

test_smallfiles:
	@echo "***  Basic small files test"
	./smallfiles ${TMPMP}
	${MAKE} TMPMP=${TMPMP} VND=${VND} MPART=${MPART} fsck

# rename(a, b) where a is a hard-link to b.
test_rename_hardlink:
	@echo "***  Hard link rename"
	cd ${TMPMP} && : > foo && ln foo bar && mv foo bar
	sync
	rm -f bar

# Trivial newfs & fsck test
test_mkfs_mount:
	VND=${VND} TMPMP=${TMPMP} TMPIM=${TMPIM} SRCDIR=${SRCDIR} MPART=${MPART} RPART=${RPART} ./mkfs_mount

#
# Utility targets.
#
fsck:
	umount ${TMPMP}
	fsck_lfs -n ${CVND}${MPART}
	mount ${BVND}${MPART} ${TMPMP}

clean:
	-umount ${TMPMP}
	vnconfig -u ${BVND}${RPART}
	rmdir ${TMPMP}
	rm -f ${TMPIM}

.include <bsd.prog.mk>
