#	$NetBSD: Makefile,v 1.1.10.1 2019/06/10 22:09:44 christos Exp $

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
