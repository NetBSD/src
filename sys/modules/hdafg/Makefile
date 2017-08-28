#	$NetBSD: Makefile,v 1.5.2.1 2017/08/28 17:53:10 skrll Exp $

.include "../Makefile.inc"
.include "${.CURDIR}/Makefile.inc"

KMOD=	hdafg

# For non-rump modules, enable HDMI audio

CPPFLAGS+=	-DHDAUDIO_ENABLE_HDMI

.include <bsd.kmodule.mk>
