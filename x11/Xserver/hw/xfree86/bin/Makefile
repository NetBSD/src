#	$NetBSD: Makefile,v 1.1 2003/12/26 08:22:10 rtr Exp $

SUBDIR=	xf86config xf86cfg kbd_mode scanpci pcitweak mmapr mmapw gtf

.if ${MACHINE} == "i386"
SUBDIR+=ioport
.endif

XTODO=	matchagp

.include <bsd.subdir.mk>
