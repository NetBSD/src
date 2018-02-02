#	$NetBSD: Makefile,v 1.16 2018/02/02 01:02:40 mrg Exp $

DRIVER=		xf86-video-sis
DRIVER_NAME=	sis_drv

SRCS=		init301.c init.c initextx.c  sis300_accel.c
SRCS+=		sis310_accel.c sis6326_video.c sis_accel.c sis_cursor.c
SRCS+=		sis_dac.c sis_dga.c sis_driver.c sis_memcpy.c sis_opt.c
SRCS+=		sis_setup.c sis_shadow.c sis_utility.c sis_vb.c
SRCS+=		sis_vga.c sis_video.c
MAN=		sis.4

CPPFLAGS+=	-DHAVE_CONFIG_H -I${X11SRCDIR.${DRIVER}}/../include
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/src
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm

X11MANCPP=		yes

CWARNFLAGS.clang+=	-Wno-parentheses

.include "../Makefile.xf86-driver"

# XXX
COPTS.sis_driver.c+=	${${ACTIVE_CC} == "gcc":? -Wno-error=discarded-qualifiers :}
COPTS.sis_utility.c+=	${${ACTIVE_CC} == "gcc":? -Wno-error=discarded-qualifiers :}
COPTS.initextx.c+=	${${ACTIVE_CC} == "gcc":? -Wno-error=discarded-qualifiers :}
COPTS.sis_driver.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=incompatible-pointer-types-discards-qualifiers :}
COPTS.sis_utility.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=incompatible-pointer-types-discards-qualifiers :}
COPTS.initextx.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=incompatible-pointer-types-discards-qualifiers :}
