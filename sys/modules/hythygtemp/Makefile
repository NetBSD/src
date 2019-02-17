# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:50 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	hythygtemp
IOCONF=	hythygtemp.ioconf
SRCS=	hytp14.c

.if ${ACTIVE_CC} == "clang"
COPTS.hytp14.c+=	-Wno-missing-noreturn
.endif

.include <bsd.kmodule.mk>
