#	$NetBSD: Makefile,v 1.36 2022/05/22 17:55:08 rillig Exp $
#

.include <bsd.own.mk>

TESTSDIR=       ${TESTSBASE}/usr.bin

TESTS_SUBDIRS=	awk basename bzip2 cc cmp compress config cpio col cut \
		diff dirname find fstat gdb grep gzip id indent \
		infocmp jot ld locale m4 make mixerctl mkdep nbperf \
		netpgpverify patch pkill pr printf pwhash rump_server \
		shmif_dumpbus sdiff sed sort tar tmux tr unifdef uniq \
		vmstat xlint ztest

.if ${MKCXX} != "no"
TESTS_SUBDIRS+=	c++
.endif

.include <bsd.test.mk>
