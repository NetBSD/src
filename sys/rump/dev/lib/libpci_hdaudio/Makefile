#	$NetBSD: Makefile,v 1.3.16.2 2017/12/03 11:39:08 jdolecek Exp $
#

RUMPTOP=${TOPRUMP}

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/sys/modules/hdaudio/Makefile.inc"
.include "${NETBSDSRCDIR}/sys/modules/hdaudio_pci/Makefile.inc"

LIB=	rumpdev_pci_hdaudio
COMMENT=HDaudio PCI attachment
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
