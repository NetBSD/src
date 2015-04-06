#	$NetBSD: Makefile,v 1.1.2.1 2015/04/06 15:18:25 skrll Exp $
#

RUMPTOP=${TOPRUMP}

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/sys/modules/hdaudio/Makefile.inc"
.include "${NETBSDSRCDIR}/sys/modules/hdaudio_pci/Makefile.inc"

LIB=	rumpdev_pci_hdaudio
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
