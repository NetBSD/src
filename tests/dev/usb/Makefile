#	$NetBSD: Makefile,v 1.2 2016/01/06 15:34:54 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/usb

TESTS_C=	t_hid

CPPFLAGS.t_hid.c=	-I${.CURDIR}/../../../sys/dev/usb

SUBDIR=	libhid
LIBHIDDIR!= cd ${.CURDIR}/libhid && ${PRINTOBJDIR}
LDFLAGS+= -L${LIBHIDDIR}
LDADD+= -Wl,--whole-archive -lrumpdev_hid -Wl,--no-whole-archive

LDADD+= -lrump
LDADD+= -lrumpuser
LDADD+= -lrump
LDADD+= -lpthread

.include <bsd.test.mk>
