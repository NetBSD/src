#	$NetBSD: Makefile,v 1.49 2023/06/03 09:09:17 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/12/93

.include <bsd.own.mk>

USE_FORT?= yes	# setgid

ROUTE=${NETBSDSRCDIR}/sbin/route
PF=${NETBSDSRCDIR}/sys/dist/pf

.PATH: ${ROUTE} 

RUMPPRG=netstat
SRCS=	atalk.c bpf.c fast_ipsec.c if.c inet.c inet6.c \
	main.c mbuf.c mroute.c mroute6.c pfkey.c pfsync.c route.c \
	unix.c vtw.c rtutil.c
BINGRP=	kmem
BINMODE=2555
LDADD=	-lutil -lkvm
DPADD=	${LIBUTIL} ${LIBKVM}
CPPFLAGS+= -DIPSEC -I${.CURDIR}
CPPFLAGS+= -I${PF}
CPPFLAGS+= -I${ROUTE}

.if (${MKRUMP} != "no")
.PATH:  ${.CURDIR}/../../lib/libc/gen
RUMPSRCS+= sysctlbyname.c sysctlgetmibinfo.c sysctlnametomib.c
LDADD.rump+=	-lrumpres
DPADD.add+=	${LIBRUMPRES}
CPPFLAGS+=      -DRUMP_ACTION

SANITIZER_RENAME_CLASSES+=	rump
SANITIZER_RENAME_FILES.rump+=	${PROG}_rumpops.c ${RUMPSRCS}
SANITIZER_RENAME_SYMBOL.rump+=	sysctlbyname sysctlgetmibinfo sysctlnametomib
.endif


.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

.include "../../compat/exec.mk"

COPTS.atalk.c+=		${CC_WNO_FORMAT_TRUNCATION}
COPTS.inet6.c+=		${CC_WNO_STRINGOP_TRUNCATION}
COPTS.rtutil.c+=	${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
