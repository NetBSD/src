#	$NetBSD: Makefile,v 1.42 2015/05/15 06:58:59 ozaki-r Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/12/93

.include <bsd.own.mk>

USE_FORT?= yes	# setgid

RUMPPRG=netstat
SRCS=	atalk.c bpf.c fast_ipsec.c if.c inet.c inet6.c \
	main.c mbuf.c mroute.c mroute6.c pfkey.c pfsync.c route.c \
	unix.c vtw.c rtutil.c
BINGRP=	kmem
BINMODE=2555
LDADD=	-lutil -lkvm
DPADD=	${LIBUTIL} ${LIBKVM}
CPPFLAGS+= -DIPSEC -I${.CURDIR}
CPPFLAGS+= -I${NETBSDSRCDIR}/sys/dist/pf
CPPFLAGS+= -I${NETBSDSRCDIR}/sbin/route

CWARNFLAGS.clang+=	-Wno-format
COPTS.show.c += -Wno-format-nonliteral

.PATH:  ${.CURDIR}/../../lib/libc/gen
.PATH:  ${.CURDIR}/../../lib/libc/net
.PATH:  ${.CURDIR}/../../sbin/route
CPPFLAGS+= -DRUMP_ACTION
RUMPSRCS+= sysctlbyname.c sysctlgetmibinfo.c sysctlnametomib.c
RUMPSRCS+= if_indextoname.c getifaddrs.c getnameinfo.c

.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

.include <bsd.prog.mk>
