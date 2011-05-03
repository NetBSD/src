#	$NetBSD: Makefile,v 1.34 2011/05/03 18:28:46 dyoung Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/12/93

.include <bsd.own.mk>

USE_FORT?= yes	# setgid

RUMPPRG=netstat
SRCS=	atalk.c bpf.c fast_ipsec.c if.c inet.c inet6.c ipsec.c iso.c \
	main.c mbuf.c mroute.c mroute6.c pfsync.c show.c route.c tp_astring.c \
	unix.c vtw.c
.PATH:	${NETBSDSRCDIR}/sys/netiso 
BINGRP=	kmem
BINMODE=2555
LDADD=	-lkvm
DPADD=	${LIBKVM}
CPPFLAGS+= -DIPSEC
CPPFLAGS+= -I${NETBSDSRCDIR}/sys/dist/pf

.PATH:  ${.CURDIR}/../../lib/libc/gen
.PATH:  ${.CURDIR}/../../lib/libc/net
CPPFLAGS+= -DRUMP_ACTION
RUMPSRCS+= sysctlbyname.c sysctlgetmibinfo.c sysctlnametomib.c
RUMPSRCS+= if_indextoname.c getifaddrs.c

.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

.include <bsd.prog.mk>
