#	$NetBSD: Makefile,v 1.2 2004/01/23 08:45:02 rtr Exp $

SUBDIR=	xf86config xf86cfg kbd_mode scanpci pcitweak mmapr mmapw gtf

.if ${MACHINE} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport
.endif

XTODO=	matchagp

.include <bsd.subdir.mk>
