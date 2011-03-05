#	$NetBSD: Makefile,v 1.1.2.2 2011/03/05 15:10:43 bouyer Exp $

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
