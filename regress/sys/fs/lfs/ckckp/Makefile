#	$NetBSD: Makefile,v 1.7 2006/11/11 14:47:27 jmmv Exp $
#
# Note: this test must be run with LFSDEV defined, e.g. "LFSDEV=wd0g".
# It will destroy the existing contents of $LFSDEV to perform the test.
# The test also requires the availability of two files each the size
# of ${LFSDEV} to store temporary filesystem images in, during the course
# of the run.
#
# This test requires that Perl be installed to run.
#

NOMAN=	# defined
TMPMP= /tmp/lfsregresstest_mount
SRCDIR?= ${.CURDIR}
CLEAN= clean
LFSBDEV="/dev/${LFSDEV}"
LFSCDEV="/dev/r${LFSDEV}"
GFILE?= "good_fs.img"		# "Good" file - will be as large as the FS
WFILE?= "work_fs.img"		# Work file - will also be as large as the FS
COUNT?= 5
PLEX?=  2
FSIZE?= 16384

regress: test_ckckp

.ifndef REGRESS_LOG
REGRESS_LOG=/dev/null
.endif

test_ckckp: ckckp cleanalot_async run_ckckp
.ifndef LFSDEV
	@echo "***  Checkpoint validity test skipped"
	@echo ${.CURDIR} SKIPPED "LFSDEV not defined" >> ${REGRESS_LOG}
.else
	@echo "***  Checkpoint validity test"
	@echo "***  WARNING: will newfs_lfs ${LFSCDEV} in 10 seconds"
	@sleep 10
	newfs_lfs ${LFSCDEV}
	-mkdir ${TMPMP}
	mount -o-n ${LFSBDEV} ${TMPMP}
	/libexec/lfs_cleanerd -b -n4 -t5 ${TMPMP}
	./run_ckckp ${TMPMP} ${LFSCDEV} ${GFILE} ${WFILE} ${PLEX} ${COUNT} ${FSIZE} || \
		echo ${.CURDIR} FAILED | tee -a ${REGRESS_LOG}
	umount ${TMPMP}
	rmdir ${TMPMP}
	echo ${.CURDIR} PASSED | tee -a ${REGRESS_LOG}
.endif

abort:
	-kill -9 `ps auxww | grep ckckp | grep -v grep | awk '{print $$2}'`
	umount ${TMPMP}

clean:
	rm -f *.o ckckp cleanalot_async *~ *.core check-all.log ${GFILE} ${WFILE}

.include <bsd.prog.mk>
