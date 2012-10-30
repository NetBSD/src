#	$NetBSD: Makefile,v 1.7.6.1 2012/10/30 18:59:52 yamt Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev

TESTS_SUBDIRS+=	cgd raidframe
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=	audio md scsipi sysmon
.endif


.include <bsd.test.mk>
