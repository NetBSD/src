#	$NetBSD: Makefile,v 1.39.2.1 2009/05/13 19:19:02 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

# when making a change to this file, please check if the change is
# also needed for src/distrib/utils/x_ifconfig/Makefile.

.include <bsd.own.mk>

MAN=	ifconfig.8

#CFLAGS+=-g
SRCS= af_atalk.c af_iso.c af_link.c carp.c
.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+= af_inet6.c
.endif

.include "Makefile.inc"

.if ${MACHINE_ARCH} == "m68000"
# XXX workaround for gcc -O1 bug (PR bin/40036 and toolchain/40066)
COPTS.ifconfig.c+= -fno-loop-optimize
.endif

.include <bsd.prog.mk>
