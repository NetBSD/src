# $NetBSD: Makefile,v 1.7 2010/05/31 23:36:12 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern

TESTS_C=	t_modcmd
TESTS_C+=	t_modlinkset
TESTS_C+=	t_threads
TESTS_C+=	t_tsleep
TESTS_C+=	t_vm

ADD_TO_LD=	-lrumpvfs -lrump -lrumpuser -lpthread
LDADD.t_modlinkset+=	-lukfs -lrumpdev_disk -lrumpdev -lrumpfs_msdos 
LDADD.t_modlinkset+=	-lrumpfs_cd9660 ${ADD_TO_LD}
LDADD+=			${ADD_TO_LD}

KERNSPACE != cd ${.CURDIR}/../kernspace && ${PRINTOBJDIR}
LDADD+=	-L${KERNSPACE} -lkernspace

WARNS=	4

.include <bsd.test.mk>
