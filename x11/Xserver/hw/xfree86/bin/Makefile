#	$NetBSD: Makefile,v 1.5 2009/02/14 13:48:17 abs Exp $

SUBDIR=	xf86config xf86cfg kbd_mode scanpci pcitweak mmapr mmapw gtf

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport
.endif

XTODO=	matchagp

.include <bsd.subdir.mk>
