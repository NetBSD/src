#	$NetBSD: Makefile,v 1.8 2016/09/05 06:30:40 martin Exp $

DRIVER=		xf86-video-suncg6
DRIVER_NAME=	suncg6_drv

SRCS=		cg6_driver.c cg6_cursor.c cg6_accel.c cg6_exa.c

CPPFLAGS+=	${X11FLAGS.DRI}

MAN=		suncg6.4

.include "../Makefile.xf86-driver"

# XXX
.if ${HAVE_GCC:U0} > 48
COPTS.cg6_accel.c+=	${${ACTIVE_CC} == "gcc":? -Wno-error=discarded-qualifiers :}
.endif
