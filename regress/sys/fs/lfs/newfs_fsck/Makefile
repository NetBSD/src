#	$NetBSD: Makefile,v 1.2 2003/01/29 05:39:41 perseant Exp $

NOMAN=	# defined

TMPL=/tmp/lfstemplate
TMPMP=/tmp/lfsregresstest_mount
TMPIM=/var/tmp/lfsregresstest.im
SRCDIR?= ${.CURDIR}

VND?= vnd0
BVND= /dev/${VND}
CVND= /dev/r${VND}
MPART?= a
.if (${MACHINE_ARCH} == "i386")
RPART?= d
.else
RPART?= c
.endif

#
# Make a small filesystem, and fsck it.
# This ensures that newfs_lfs and fsck_lfs play well together.
#
# Mount it, copy some data in, unmount, fsck.
# This ensures that kernel and fsck_lfs play well together.
#
# This was based on the FFS endianness regression tests.
#
regress:
	@echo "***  Creating a dummy directory tree at ${TMPMP} monted on" \
	   "${TMPIM}."
	dd if=/dev/zero of=${TMPIM} count=5860
	vnconfig -v ${BVND}${RPART} ${TMPIM}
	disklabel -f ${SRCDIR}/disktab -rw ${VND} floppy288
	newfs_lfs -B 131072 -b 4096 -f 512 ${CVND}${MPART}
	@echo "***  Checking that newfs_lfs could be understood by fsck_lfs"
	fsck_lfs -n ${CVND}${MPART}
	mkdir ${TMPMP}
	mount -t lfs ${BVND}${MPART} ${TMPMP}
	for i in 0 1 2 3 4 5 6 7 8 9; do dd if=/dev/zero of=${TMPMP}/zero bs=16384 count=50; sync; done
	umount ${TMPMP}
	@echo "***  Checking that kernel write could be understood by fsck_lfs"
	fsck_lfs -n ${CVND}${MPART}
	vnconfig -u ${BVND}${RPART}

clean:
	-umount ${TMPMP}
	vnconfig -u ${BVND}${RPART}
	rmdir ${TMPMP}

.include <bsd.prog.mk>
