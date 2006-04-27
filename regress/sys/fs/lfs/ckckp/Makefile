#	$NetBSD: Makefile,v 1.3 2006/04/27 22:37:54 perseant Exp $
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
COUNT?= 20
PLEX?=  2

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
	mount ${LFSBDEV} ${TMPMP}
	./run_ckckp ${TMPMP} ${LFSCDEV} ${GFILE} ${WFILE} ${PLEX} ${COUNT} || \
		echo ${.CURDIR} FAILED | tee -a ${REGRESS_LOG}
	umount ${TMPMP}
	rmdir ${TMPMP}
	echo ${.CURDIR} PASSED | tee -a ${REGRESS_LOG}
.endif

abort: go
	-kill -9 `ps auxww | grep ckckp | grep -v grep | awk '{print $$2}'`
	./go ${TMPMP}
	umount ${TMPMP}

clean:
	rm -f ckckp cleanalot_async go *~ *.core ${GFILE} ${WFILE}

.include <bsd.prog.mk>
