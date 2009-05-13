#	$NetBSD: Makefile,v 1.4.26.1 2009/05/13 19:20:47 jym Exp $

SUBDIR=	xf86config xf86cfg kbd_mode scanpci pcitweak mmapr mmapw gtf

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport
.endif

XTODO=	matchagp

.include <bsd.subdir.mk>
