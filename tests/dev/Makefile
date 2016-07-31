#	$NetBSD: Makefile,v 1.10.2.1 2016/07/31 01:33:21 pgoyette Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev

TESTS_SUBDIRS+=	cgd fss raidframe
.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE) 
TESTS_SUBDIRS+=	audio md scsipi sysmon usb
.endif


.include <bsd.test.mk>
