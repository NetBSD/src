#	$NetBSD: Makefile,v 1.3 2019/11/20 19:37:54 pgoyette Exp $

.include "../Makefile.inc"

KMOD=	compat_aoutm68k

CPPFLAGS+=	-DEXEC_AOUT
CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG -DKTRACE
CPPFLAGS+=	-DCOMPAT_43 -DCOMPAT_12 -DCOMPAT_13 -DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_AOUT_M68K -DQUOTA

.PATH:	${S}/compat/aoutm68k
SRCS+=	aoutm68k_exec.c aoutm68k_mod.c aoutm68k_stat.c aoutm68k_sysent.c

.include <bsd.kmodule.mk>
