#	$NetBSD: Makefile,v 1.10 2016/01/05 17:22:38 jakllsch Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev

TESTS_SUBDIRS+=	cgd raidframe
.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE) 
TESTS_SUBDIRS+=	audio md scsipi sysmon usb
.endif


.include <bsd.test.mk>
