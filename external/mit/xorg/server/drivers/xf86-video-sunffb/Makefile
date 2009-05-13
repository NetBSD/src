#	$NetBSD: Makefile,v 1.2.2.1 2009/05/13 18:53:28 jym Exp $

DRIVER=		xf86-video-sunffb
DRIVER_NAME=	sunffb_drv

SRCS=		ffb_accel.c ffb_dac.c ffb_driver.c ffb_wid.c ffb_attr.c \
		ffb_ddc.c ffb_cursor.c ffb_dga.c \
		VISmoveImage.s ffb_asm.s

.if ${MACHINE_ARCH} == "sparc64"
AFLAGS+= -x assembler-with-cpp -Wa,-Av9a
.endif

.if ${MACHINE_ARCH} == "sparc"
AFLAGS+= -x assembler-with-cpp -Wa,-Av8plusa
COPTS= -mv8plus
.endif

CPPFLAGS+=	${X11FLAGS.DRI} -DUSE_VIS

MAN=		sunffb.4

.include "../Makefile.xf86-driver"
