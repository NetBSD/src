#	$NetBSD: Makefile,v 1.1.24.1 2006/06/21 15:10:24 yamt Exp $

SUBDIR=		vnd

.if ${MACHINE} == "i386" || ${MACHINE} == "alpha"
SUBDIR+=	pci
.endif

.if ${MACHINE} == "i386" || ${MACHINE} == "alpha"
SUBDIR+=	pcmcia
.endif

.include <bsd.subdir.mk>
