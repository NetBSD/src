#	$NetBSD: Makefile,v 1.7 2020/03/01 18:08:13 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/md
FILESDIR=	${TESTSDIR}

TESTS_SH=	t_md

BINDIR=		${TESTSDIR}
MKMAN=no

PROGS=		h_mdserv

CPPFLAGS+=	-D_KERNTYPES
LDADD+=	-lrumpdev_md -lrumpdev_disk -lrumpdev
LDADD+=	-lrumpkern_sysproxy ${LIBRUMPBASE}

WARNS=	4
NOMAN=

.include <bsd.test.mk>
