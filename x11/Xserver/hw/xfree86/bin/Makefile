#	$NetBSD: Makefile,v 1.3 2005/03/23 01:00:32 rtr Exp $

SUBDIR=	getconfig xf86config xf86cfg kbd_mode scanpci pcitweak mmapr mmapw gtf

.if ${MACHINE} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport
.endif

XTODO=	matchagp

.include <bsd.subdir.mk>
