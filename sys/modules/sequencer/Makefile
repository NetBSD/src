#	$NetBSD: Makefile,v 1.1.10.2 2017/12/03 11:38:59 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev

KMOD=	sequencer
IOCONF=	sequencer.ioconf
SRCS=	sequencer.c

CPPFLAGS+=	-DNSEQUENCER=1 -DNMIDI=1

# Rather than our usual WARNS=4, we need to use 3, since there are a
# lot of signed-vs-unsigned compares

WARNS=  3

.include <bsd.kmodule.mk>
