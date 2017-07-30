#	$NetBSD: Makefile,v 1.2 2017/07/30 00:53:57 nat Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev

KMOD=	audio
IOCONF=	audio.ioconf
SRCS=	audio.c \
	auconv.c \
	aurateconv.c \
	auvolconv.c \
	mulaw.c

CPPFLAGS+=	-DNAUDIO=1 -DNAURATECONV=1 -DNMULAW=1

.include <bsd.kmodule.mk>
