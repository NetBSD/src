#	$NetBSD: Makefile,v 1.2 2019/02/17 04:06:01 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/wscons

CPPFLAGS+=	-DNWSMUX=1

KMOD=	wsbell
IOCONF=	wsbell.ioconf
SRCS=	wsbell.c

.if ${ACTIVE_CC} == "clang"
COPTS.wsbell.c+=	-Wno-missing-noreturn
.endif

.include <bsd.kmodule.mk>
