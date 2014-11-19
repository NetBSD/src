#	$NetBSD: Makefile,v 1.1 2014/11/19 00:18:58 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/sys/modules/hdaudio/Makefile.inc"

LIB=	rumpdev_pci_hdaudio
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
