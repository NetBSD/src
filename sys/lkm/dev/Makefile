#	$NetBSD: Makefile,v 1.1.8.1 2005/11/10 14:10:25 skrll Exp $

SUBDIR=		vnd

.if ${MACHINE} == "i386" || ${MACHINE} == "alpha"
SUBDIR+=	pci
.endif

.if ${MACHINE} == "i386" || ${MACHINE} == "alpha"
SUBDIR+=	pcmcia
.endif

.include <bsd.subdir.mk>
