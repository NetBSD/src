#	$NetBSD: Makefile,v 1.3 2022/06/04 03:31:10 pgoyette Exp $ */

.include "../Makefile.inc"

.PATH:	${S}/dev

KMOD=	midi
SRCS=	midi_mod.c

CPPFLAGS+=	-DNMIDI=1 -DNSEQUENCER=1

.include <bsd.kmodule.mk>
