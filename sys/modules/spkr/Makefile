# $NetBSD: Makefile,v 1.4 2016/12/09 02:22:56 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	spkr
IOCONF=	spkr.ioconf
SRCS=	spkr.c spkr_pcppi.c

WARNS=	4

.include <bsd.kmodule.mk>
