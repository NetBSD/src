#	$NetBSD: Makefile,v 1.44 2025/11/28 23:58:47 nia Exp $
#

.include <bsd.own.mk>

TESTSDIR=       ${TESTSBASE}/usr.bin

TESTS_SUBDIRS=	awk basename bzip2 cc cmp compress config cpio col cut \
		diff diff3 dirname error find fstat ftp gcov gdb grep gzip id \
		indent infocmp jot ld locale m4 make mixerctl mkdep mtree \
		nbperf netpgpverify patch pkill pr printf pwhash realpath \
		rump_server shmif_dumpbus shmif_pcapin sdiff sed sort stat \
		tar tmux tr unifdef uniq vmstat xlint ztest

.if ${MKCXX} != "no"
TESTS_SUBDIRS+=	c++
.endif

.include <bsd.test.mk>
