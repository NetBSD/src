#	$NetBSD: Makefile,v 1.1.2.7 2018/03/18 21:41:31 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_60

CPPFLAGS+=	-DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DCPU_UCODE
.endif

SRCS+=	compat_60_mod.c kern_sa_60.c tty_60.c kern_time_60.c ccd_60.c

.include <bsd.kmodule.mk>
