#	$NetBSD: Makefile,v 1.4.12.1 2017/12/03 11:38:54 jdolecek Exp $

.include "../Makefile.inc"
.include "${.CURDIR}/Makefile.inc"

KMOD=	hdafg

# For non-rump modules, enable HDMI audio

CPPFLAGS+=	-DHDAUDIO_ENABLE_HDMI

.include <bsd.kmodule.mk>
