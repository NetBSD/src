#	$NetBSD: Makefile,v 1.3 2019/06/01 06:59:17 kre Exp $
#

PROG= t_hid
NOMAN=

.PATH: ${.CURDIR}/..

CPPFLAGS.t_hid.c= -I${.CURDIR}/../../../../sys/dev/hid
CPPFLAGS.t_hid.c+= -I${.CURDIR}/../../../../sys/dev/usb
CPPFLAGS.t_hid.c+= -D_RUMPKERNEL

.include <bsd.own.mk>

BINDIR= ${TESTSBASE}/dev/usb

LIBHIDDIR!= cd ${.CURDIR}/../libhid && ${PRINTOBJDIR}
LDFLAGS+= -L${LIBHIDDIR}
LDADD+= -Wl,--whole-archive -lrumpdev_hid -Wl,--no-whole-archive
DPADD+= ${LIBHIDDIR}/librumpdev_hid.a
DPADD+= ${ATF_C}

LDADD+= -latf-c
LDADD+= -lrump
LDADD+= -lrumpuser
LDADD+= -lrumpvfs
LDADD+= -lpthread

.include <bsd.prog.mk>
