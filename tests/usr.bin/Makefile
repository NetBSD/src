#	$NetBSD: Makefile,v 1.25.10.2 2020/04/13 08:05:32 martin Exp $
#

.include <bsd.own.mk>

TESTSDIR=       ${TESTSBASE}/usr.bin

TESTS_SUBDIRS=	awk basename bzip2 cc cmp config cpio cut \
		diff dirname find gdb grep gzip id indent \
		infocmp jot ld m4 make mixerctl mkdep nbperf netpgpverify \
		pkill pr printf pwhash rump_server shmif_dumpbus sdiff \
		sed sort tar tmux tr unifdef uniq vmstat xlint

.if ${MKCXX} != "no"
TESTS_SUBDIRS+=	c++
.endif

.if (defined(MKARGON2) && ${MKARGON2} != "no")
TESTS_SUBDIRS+=	argon2
.endif


.include <bsd.test.mk>
