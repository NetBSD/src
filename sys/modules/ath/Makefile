#	$NetBSD: Makefile,v 1.1.8.2 2011/06/06 09:09:46 jruoho Exp $

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
