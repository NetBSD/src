#	$NetBSD: Makefile,v 1.3 2005/07/23 16:11:18 drochner Exp $

SUBDIR=		vnd

.if ${MACHINE} == "i386" || ${MACHINE} == "alpha"
SUBDIR+=	pci
.endif

.if ${MACHINE} == "i386" || ${MACHINE} == "alpha"
SUBDIR+=	pcmcia
.endif

.include <bsd.subdir.mk>
