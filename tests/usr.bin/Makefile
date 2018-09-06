#	$NetBSD: Makefile,v 1.25.8.1 2018/09/06 06:56:48 pgoyette Exp $
#

.include <bsd.own.mk>

TESTSDIR=       ${TESTSBASE}/usr.bin

TESTS_SUBDIRS=	awk basename bzip2 cc cmp config cut \
		diff dirname find gdb grep gzip id \
		infocmp jot ld m4 make mixerctl mkdep nbperf netpgpverify \
		pkill pr printf rump_server shmif_dumpbus sdiff \
		sed sort tmux tr unifdef uniq vmstat xlint

.if ${MKCXX} != "no"
TESTS_SUBDIRS+=	c++
.endif

.include <bsd.test.mk>
