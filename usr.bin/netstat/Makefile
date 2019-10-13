#	$NetBSD: Makefile,v 1.47 2019/10/13 07:28:16 mrg Exp $
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

SANITIZER_RENAME_CLASSES+=	rump
SANITIZER_RENAME_FILES.rump+=	${PROG}_rumpops.c ${RUMPSRCS}
SANITIZER_RENAME_SYMBOL.rump+=	sysctlbyname sysctlgetmibinfo sysctlnametomib
.endif


.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

.include "../../compat/exec.mk"

COPTS.atalk.c+=		${GCC_NO_FORMAT_TRUNCATION}
COPTS.inet6.c+=		${GCC_NO_STRINGOP_TRUNCATION}
COPTS.rtutil.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
