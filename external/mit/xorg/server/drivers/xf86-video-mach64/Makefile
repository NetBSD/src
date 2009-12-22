#	$NetBSD: Makefile,v 1.5 2009/12/22 09:12:42 nakayama Exp $

DRIVER=		xf86-video-mach64
DRIVER_NAME=	mach64_drv

SRCS=		atibus.c atichip.c atiprobe.c atividmem.c \
		atiadjust.c atiaudio.c aticlock.c aticonfig.c \
		aticonsole.c atidac.c atidecoder.c atidsp.c \
		atii2c.c atilock.c atimach64.c atimach64accel.c \
		atimach64cursor.c atimach64i2c.c atimach64io.c \
		atimach64xv.c atimode.c atipreinit.c atiprint.c \
		atirgb514.c atiscreen.c atituner.c atiutil.c \
		ativalid.c atiload.c atimisc.c atimach64probe.c \
		${SRCS.cpio} ${SRCS.dga} #${SRCS.dri} ${SRCS.exa}

SRCS.cpio=	ativga.c ativgaio.c atibank.c atiwonder.c atiwonderio.c
SRCS.dga=	atidga.c
SRCS.dri=	atidri.c
SRCS.exa=	atimach64exa.c

CPPFLAGS+=	-DUSE_XAA

.if ${MACHINE_ARCH} == "powerpc" || ${MACHINE_ARCH} == "sparc64" || \
    ${MACHINE_ARCH} == "sparc"
CPPFLAGS+= -DAVOID_CPIO
.endif

MAN=		# defined

.include "../Makefile.xf86-driver"
