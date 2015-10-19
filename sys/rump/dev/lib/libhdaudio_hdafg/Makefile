#	$NetBSD: Makefile,v 1.2 2015/10/19 16:16:33 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/sys/modules/hdafg/Makefile.inc"

LIB=	rumpdev_hdaudio_hdafg
COMMENT=High Definition Audio (hdaudio) driver
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
