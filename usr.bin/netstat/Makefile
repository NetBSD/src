#	$NetBSD: Makefile,v 1.39 2013/03/01 18:26:11 joerg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/12/93

.include <bsd.own.mk>

USE_FORT?= yes	# setgid

RUMPPRG=netstat
SRCS=	atalk.c bpf.c fast_ipsec.c if.c inet.c inet6.c \
	main.c mbuf.c mroute.c mroute6.c pfkey.c pfsync.c show.c route.c \
	unix.c vtw.c
BINGRP=	kmem
BINMODE=2555
LDADD=	-lkvm
DPADD=	${LIBKVM}
CPPFLAGS+= -DIPSEC
CPPFLAGS+= -I${NETBSDSRCDIR}/sys/dist/pf

CWARNFLAGS.clang+=	-Wno-format
COPTS.show.c += -Wno-format-nonliteral

.PATH:  ${.CURDIR}/../../lib/libc/gen
.PATH:  ${.CURDIR}/../../lib/libc/net
CPPFLAGS+= -DRUMP_ACTION
RUMPSRCS+= sysctlbyname.c sysctlgetmibinfo.c sysctlnametomib.c
RUMPSRCS+= if_indextoname.c getifaddrs.c

.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

.include <bsd.prog.mk>
