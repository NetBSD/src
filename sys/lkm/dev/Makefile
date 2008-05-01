#	$NetBSD: Makefile,v 1.5 2008/05/01 21:51:20 xtraeme Exp $

SUBDIR=		vnd

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SUBDIR+=	isa
.endif

.if ${MACHINE} == "i386" || ${MACHINE} == "alpha"
SUBDIR+=	pci pcmcia
.endif

.include <bsd.subdir.mk>
