#	$NetBSD: Makefile,v 1.4 2005/12/11 12:24:47 christos Exp $

SUBDIR=		vnd

.if ${MACHINE} == "i386" || ${MACHINE} == "alpha"
SUBDIR+=	pci
.endif

.if ${MACHINE} == "i386" || ${MACHINE} == "alpha"
SUBDIR+=	pcmcia
.endif

.include <bsd.subdir.mk>
