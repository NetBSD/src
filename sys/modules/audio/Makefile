#	$NetBSD: Makefile,v 1.1 2017/06/01 09:58:27 pgoyette Exp $

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

# Rather than our usual WARNS=4, we need to use 3, since there are a
# lot of signed-vs-unsigned compares

WARNS=	3

.include <bsd.kmodule.mk>
