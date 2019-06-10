#	$NetBSD: Makefile,v 1.6.6.1 2019/06/10 22:09:25 christos Exp $

.include "../Makefile.inc"
.include "${.CURDIR}/Makefile.inc"

KMOD=	hdafg

# For non-rump modules, enable HDMI audio

CPPFLAGS+=	-DHDAUDIO_ENABLE_HDMI

WARNS=	3

.include <bsd.kmodule.mk>
