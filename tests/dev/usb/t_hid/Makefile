#	$NetBSD: Makefile,v 1.5 2020/03/01 20:19:54 christos Exp $
#

NOMAN=
.include <bsd.own.mk>

PROG= t_hid

.PATH: ${.CURDIR}/..

CPPFLAGS.t_hid.c= -I${.CURDIR}/../../../../sys/dev/hid
CPPFLAGS.t_hid.c+= -I${.CURDIR}/../../../../sys/dev/usb
CPPFLAGS.t_hid.c+= -D_RUMPKERNEL

BINDIR= ${TESTSBASE}/dev/usb

LIBHIDDIR!= cd ${.CURDIR}/../libhid && ${PRINTOBJDIR}
LDFLAGS+= -L${LIBHIDDIR}
LDADD+= -Wl,--whole-archive -lrumpdev_hid -Wl,--no-whole-archive
DPADD+= ${LIBHIDDIR}/librumpdev_hid.a
DPADD+= ${ATF_C} 

LDADD+= ${LIBRUMPBASE} -latf-c

.include <bsd.prog.mk>
