#	$NetBSD: Makefile,v 1.12 2016/09/27 19:24:19 joerg Exp $

DRIVER=		xf86-video-sunffb
DRIVER_NAME=	sunffb_drv

SRCS=		ffb_accel.c ffb_dac.c ffb_driver.c ffb_wid.c ffb_attr.c \
		ffb_ddc.c ffb_cursor.c ffb_dga.c ffb_exa.c \
		ffb_accel_common.c \
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

# XXX
.if ${HAVE_GCC:U0} > 48
COPTS.ffb_dac.c+=	${${ACTIVE_CC} == "gcc":? -Wno-error=discarded-qualifiers :}
COPTS.ffb_dga.c+=	${${ACTIVE_CC} == "gcc":? -Wno-error=discarded-qualifiers :}
.endif
COPTS.ffb_dac.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=incompatible-pointer-types-discards-qualifiers :}
COPTS.ffb_dga.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=incompatible-pointer-types-discards-qualifiers :}
COPTS.ffb_exa.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=pointer-sign :}
