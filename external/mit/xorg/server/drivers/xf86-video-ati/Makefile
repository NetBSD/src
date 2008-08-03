#	$NetBSD: Makefile,v 1.3 2008/08/03 09:24:14 rtr Exp $

DRIVER=		xf86-video-ati
DRIVER_NAME=	ati_drv

SRCS=		ati.c atimodule.c
MAN=		ati.4

CPPFLAGS.atimodule.c=	-DPACKAGE_VERSION_MAJOR=${PACKAGE_MAJOR}
CPPFLAGS.atimodule.c+=	-DPACKAGE_VERSION_MINOR=${PACKAGE_MINOR}
CPPFLAGS.atimodule.c+=	-DPACKAGE_VERSION_PATCHLEVEL=${PACKAGE_PATCH}

.include "../Makefile.xf86-driver"
