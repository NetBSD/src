#	$NetBSD: Makefile,v 1.6 2017/06/16 04:16:18 pgoyette Exp $

.include "../Makefile.inc"
.include "${.CURDIR}/Makefile.inc"

KMOD=	hdafg

# For non-rump modules, enable HDMI audio

CPPFLAGS+=	-DHDAUDIO_ENABLE_HDMI

.include <bsd.kmodule.mk>
