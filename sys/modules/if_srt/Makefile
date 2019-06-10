# $NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:29 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_srt
IOCONF=		srt.ioconf
SRCS=		if_srt.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

WARNS=		3

.include <bsd.kmodule.mk>
