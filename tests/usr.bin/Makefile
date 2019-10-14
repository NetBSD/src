#	$NetBSD: Makefile,v 1.29 2019/10/14 03:47:19 jhigh Exp $
#

.include <bsd.own.mk>

TESTSDIR=       ${TESTSBASE}/usr.bin

TESTS_SUBDIRS=	awk basename bzip2 cc cmp config cut \
		diff dirname find gdb grep gzip id indent \
		infocmp jot ld m4 make mixerctl mkdep nbperf netpgpverify \
		pkill pr printf pwhash rump_server shmif_dumpbus sdiff \
		sed sort tmux tr unifdef uniq vmstat xlint

.if ${MKCXX} != "no"
TESTS_SUBDIRS+=	c++
.endif

.if (defined(MKARGON2) && ${MKARGON2} != "no")
TESTS_SUBDIRS+=	argon2
.endif


.include <bsd.test.mk>
