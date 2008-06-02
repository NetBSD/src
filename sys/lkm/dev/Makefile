#	$NetBSD: Makefile,v 1.4.70.1 2008/06/02 13:24:17 mjf Exp $

SUBDIR=		vnd

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SUBDIR+=	isa
.endif

.if ${MACHINE} == "i386" || ${MACHINE} == "alpha"
SUBDIR+=	pci pcmcia
.endif

.include <bsd.subdir.mk>
