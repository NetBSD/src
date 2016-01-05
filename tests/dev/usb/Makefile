#	$NetBSD: Makefile,v 1.1 2016/01/05 17:22:38 jakllsch Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/usb

TESTS_C=	t_hid

CPPFLAGS.t_hid.c=	-I${.CURDIR}/../../../sys/dev/usb

SUBDIR=	libhid
LIBHIDDIR!= cd ${.CURDIR}/libhid && ${PRINTOBJDIR}
LDFLAGS.t_hid+= -L${LIBHIDDIR}
LDADD.t_hid+= -Wl,--whole-archive -lrumpdev_hid -Wl,--no-whole-archive

LDADD+= -lrump
LDADD+= -lrumpuser
LDADD+= -lrump
LDADD+= -lpthread

.include <bsd.test.mk>
