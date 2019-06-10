# $NetBSD: Makefile,v 1.7.8.1 2019/06/10 22:09:39 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	spkr
IOCONF=	spkr.ioconf
SRCS=	spkr.c spkr_pcppi.c

.PATH:	${S}/dev
SRCS+=	spkr_audio.c

.PATH:	${S}/dev/audio
SRCS+=	audiobell.c

WARNS=	3

CPPFLAGS+=	-DNWSMUX=1

.include <bsd.kmodule.mk>
