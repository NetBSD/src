#	$NetBSD: Makefile,v 1.49 2010/11/05 16:23:56 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

# when making a change to this file, please check if the change is
# also needed for src/distrib/utils/x_ifconfig.
# such stuff should be into Makefile.inc.

.include <bsd.own.mk>

MAN=	ifconfig.8

#DBG+=-g
SRCS= af_atalk.c af_iso.c af_link.c carp.c
.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+= af_inet6.c
.endif

.include "Makefile.inc"

CPPFLAGS+=-I${NETBSDSRCDIR}/sys/dist/pf/
SRCS+= pfsync.c

.if ${MACHINE_ARCH} == "m68000"
# XXX workaround for gcc -O1 bug (PR bin/40036 and toolchain/40066)
COPTS.ifconfig.c+= -fno-loop-optimize
.endif

#
# Compile-time debug flag.  If compiled with "make RUMP_ACTION=1",
# make rump system calls to a server in another process.
#
.ifdef RUMP_ACTION
.PATH:		${.CURDIR}/../../lib/libc/net
CPPFLAGS+=	-DRUMP_SYS_NETWORKING -DRUMP_SYS_IOCTL -DRUMP_SYS_CLOSE
CPPFLAGS+=	-DRUMP_ACTION
LDADD+=		-lrumpclient
DBG=		-g
SRCS+=		getifaddrs.c
.endif

.include <bsd.prog.mk>
