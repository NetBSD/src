#	$NetBSD: Makefile,v 1.1.2.2 2010/10/22 09:23:13 uebayasi Exp $

.include "../Makefile.inc"

KMOD=	compat_aoutm68k

CPPFLAGS+=	-DEXEC_AOUT -DCOREDUMP
CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG -DKTRACE
CPPFLAGS+=	-DCOMPAT_43 -DCOMPAT_12 -DCOMPAT_13 -DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_AOUT_M68K

.PATH:	${S}/compat/aoutm68k
SRCS+=	aoutm68k_exec.c aoutm68k_mod.c aoutm68k_stat.c aoutm68k_sysent.c

.include <bsd.kmodule.mk>
