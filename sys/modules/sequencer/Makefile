#	$NetBSD: Makefile,v 1.3 2022/06/04 03:31:10 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev

KMOD=	sequencer
SRCS=	sequencer_mod.c

CPPFLAGS+=	-DNSEQUENCER=1 -DNMIDI=1

.include <bsd.kmodule.mk>
