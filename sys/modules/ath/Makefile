#	$NetBSD: Makefile,v 1.1 2011/02/20 03:58:33 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	ath

SRCS=	ath.c \
	ath_netbsd.c \
	athrate-sample.c

WARNS=		3
CPPFLAGS+=	-DINET

.include "../ath_hal/Makefile.inc"
.include <bsd.kmodule.mk>
