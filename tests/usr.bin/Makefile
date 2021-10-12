#	$NetBSD: Makefile,v 1.35 2021/10/12 17:24:36 nia Exp $
#

.include <bsd.own.mk>

TESTSDIR=       ${TESTSBASE}/usr.bin

TESTS_SUBDIRS=	awk basename bzip2 cc cmp config cpio col cut \
		diff dirname find fstat gdb grep gzip id indent \
		infocmp jot ld locale m4 make mixerctl mkdep nbperf \
		netpgpverify patch pkill pr printf pwhash rump_server \
		shmif_dumpbus sdiff sed sort tar tmux tr unifdef uniq \
		vmstat xlint ztest

.if ${MKCXX} != "no"
TESTS_SUBDIRS+=	c++
.endif

.include <bsd.test.mk>
