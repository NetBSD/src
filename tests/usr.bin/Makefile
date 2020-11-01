#	$NetBSD: Makefile,v 1.34 2020/11/01 22:28:32 christos Exp $
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

.if (defined(MKARGON2) && ${MKARGON2} != "no")
TESTS_SUBDIRS+=	argon2
.endif

.include <bsd.test.mk>
