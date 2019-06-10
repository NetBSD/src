# $NetBSD: Makefile,v 1.1.34.1 2019/06/10 22:09:26 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	hythygtemp
IOCONF=	hythygtemp.ioconf
SRCS=	hytp14.c

.if ${ACTIVE_CC} == "clang"
COPTS.hytp14.c+=	-Wno-missing-noreturn
.endif

.include <bsd.kmodule.mk>
