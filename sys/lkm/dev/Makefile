#	$NetBSD: Makefile,v 1.4.72.1 2008/05/18 12:35:23 yamt Exp $

SUBDIR=		vnd

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SUBDIR+=	isa
.endif

.if ${MACHINE} == "i386" || ${MACHINE} == "alpha"
SUBDIR+=	pci pcmcia
.endif

.include <bsd.subdir.mk>
