# $NetBSD: Makefile,v 1.5 2016/12/09 06:12:03 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa ${S}/dev

KMOD=	spkr
IOCONF=	spkr.ioconf
SRCS=	spkr.c spkr_pcppi.c

WARNS=	4

.include <bsd.kmodule.mk>
