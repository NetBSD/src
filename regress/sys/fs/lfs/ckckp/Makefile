#	$NetBSD: Makefile,v 1.1 2006/04/17 20:02:35 perseant Exp $
#
# Note: this test must be run with LFSDEV defined, e.g. "LFSDEV=wd0g".
# It will destroy the existing contents of $LFSDEV to perform the test.
# This test requires that Perl be installed to run.
#

NOMAN=	# defined
TMPMP= /tmp/lfsregresstest_mount
SRCDIR?= ${.CURDIR}
CLEAN= clean
LFSBDEV="/dev/${LFSDEV}"
LFSCDEV="/dev/r${LFSDEV}"
COUNT?= 20
PLEX?=  2

regress: test_ckckp

test_ckckp: ckckp cleanalot_async run_ckckp
	@echo "***  Checkpoint validity test"
	@echo "***  WARNING: will newfs_lfs ${LFSCDEV} in 10 seconds"
	@sleep 10
	newfs_lfs ${LFSCDEV}
	-mkdir ${TMPMP}
	mount ${LFSBDEV} ${TMPMP}
	./run_ckckp ${TMPMP} ${LFSCDEV} ${PLEX} ${COUNT}
	umount ${TMPMP}
	rmdir ${TMPMP}

clean:
	rm -f ckckp cleanalot_async *~ *.core

.include <bsd.prog.mk>
