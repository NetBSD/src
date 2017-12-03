#	$NetBSD: Makefile,v 1.2.4.2 2017/12/03 11:38:49 jdolecek Exp $

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
