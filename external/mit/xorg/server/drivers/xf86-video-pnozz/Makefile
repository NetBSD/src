#	$NetBSD: Makefile,v 1.2 2021/05/27 04:53:44 jdc Exp $

DRIVER=		xf86-video-pnozz
DRIVER_NAME=	pnozz_drv

SRCS=		pnozz_driver.c pnozz_cursor.c pnozz_accel.c pnozz_exa.c

CPPFLAGS+=	${X11FLAGS.DRI}

MAN=		pnozz.4

.include "../Makefile.xf86-driver"

# XXX
COPTS.pnozz_accel.c+=	${${ACTIVE_CC} == "gcc":? -Wno-error=discarded-qualifiers :}
COPTS.pnozz_accel.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=incompatible-pointer-types-discards-qualifiers :}
