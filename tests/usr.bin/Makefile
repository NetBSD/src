#	$NetBSD: Makefile,v 1.37 2022/07/21 09:52:48 kre Exp $
#

.include <bsd.own.mk>

TESTSDIR=       ${TESTSBASE}/usr.bin

TESTS_SUBDIRS=	awk basename bzip2 cc cmp compress config cpio col cut \
		diff dirname find fstat gdb grep gzip id indent \
		infocmp jot ld locale m4 make mixerctl mkdep nbperf \
		netpgpverify patch pkill pr printf pwhash realpath rump_server \
		shmif_dumpbus sdiff sed sort tar tmux tr unifdef uniq \
		vmstat xlint ztest

.if ${MKCXX} != "no"
TESTS_SUBDIRS+=	c++
.endif

.include <bsd.test.mk>
