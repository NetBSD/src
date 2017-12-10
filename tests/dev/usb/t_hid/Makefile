#	$NetBSD: Makefile,v 1.2 2017/12/10 20:38:14 bouyer Exp $
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
LDADD+= -lpthread

.include <bsd.prog.mk>
