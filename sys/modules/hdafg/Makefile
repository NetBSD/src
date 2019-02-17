#	$NetBSD: Makefile,v 1.7 2019/02/17 04:05:49 rin Exp $

.include "../Makefile.inc"
.include "${.CURDIR}/Makefile.inc"

KMOD=	hdafg

# For non-rump modules, enable HDMI audio

CPPFLAGS+=	-DHDAUDIO_ENABLE_HDMI

WARNS=	3

.include <bsd.kmodule.mk>
