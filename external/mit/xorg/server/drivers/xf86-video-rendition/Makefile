#	$NetBSD: Makefile,v 1.3.6.2 2008/10/19 22:41:16 haad Exp $

DRIVER=		xf86-video-rendition
DRIVER_NAME=	rendition_drv

SRCS=		accelX.c hwcursor.c rendition.c rendition_shadow.c
SRCS+=		v1krisc.c vboard.c vloaduc.c vmisc.c vmodes.c vramdac.c
MAN=		rendition.4

CPPFLAGS.vboard.c=	-DMODULEDIR=\"${X11USRLIBDIR}/modules/\"

.include "../Makefile.xf86-driver"
