#	$NetBSD: Makefile,v 1.2.8.1 2005/04/06 21:37:52 tron Exp $

SUBDIR=	getconfig xf86config xf86cfg kbd_mode scanpci pcitweak mmapr mmapw gtf

.if ${MACHINE} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport
.endif

XTODO=	matchagp

.include <bsd.subdir.mk>
