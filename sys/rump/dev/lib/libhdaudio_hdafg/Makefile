#	$NetBSD: Makefile,v 1.1.2.1 2015/12/27 12:10:09 skrll Exp $
#

RUMPTOP=${TOPRUMP}

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/sys/modules/hdafg/Makefile.inc"

LIB=	rumpdev_hdaudio_hdafg
COMMENT=High Definition Audio (hdaudio) driver
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
