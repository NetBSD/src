#	$NetBSD: Makefile,v 1.4.74.1 2008/05/16 02:25:37 yamt Exp $

SUBDIR=		vnd

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SUBDIR+=	isa
.endif

.if ${MACHINE} == "i386" || ${MACHINE} == "alpha"
SUBDIR+=	pci pcmcia
.endif

.include <bsd.subdir.mk>
