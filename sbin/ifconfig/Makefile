#	$NetBSD: Makefile,v 1.43 2009/06/02 23:41:35 pooka Exp $
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

#
# Compile-time debug flag.  If compiled with "make RUMP_ACTION=1",
# make rump system calls.  This allows to single-step ioctl commands
# to figure out where ioctl's go in the kernel.
#
.ifdef RUMP_ACTION
CPPFLAGS+=	-DRUMP_SYS_NETWORKING -DRUMP_SYS_IOCTL -DRUMP_SYS_CLOSE
CPPFLAGS+=	-DRUMP_ACTION
LDADD+=		-lrumpnet_virtif -lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=		-lrump -lrumpuser -lpthread
DBG=		-g
.endif

.include <bsd.prog.mk>
