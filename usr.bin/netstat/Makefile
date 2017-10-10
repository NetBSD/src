#	$NetBSD: Makefile,v 1.45 2017/10/10 19:31:10 christos Exp $
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

.PATH:  ${.CURDIR}/../../sbin/route

.if (${MKRUMP} != "no")
.PATH:  ${.CURDIR}/../../lib/libc/gen
RUMPSRCS+= sysctlbyname.c sysctlgetmibinfo.c sysctlnametomib.c
LDADD.rump+=	-lrumpres
DPADD.add+=	${LIBRUMPRES}
CPPFLAGS+=      -DRUMP_ACTION
.endif


.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

.include "../../compat/exec.mk"

.include <bsd.prog.mk>
