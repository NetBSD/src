#	$NetBSD: Makefile,v 1.61 2017/10/10 19:30:06 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

# when making a change to this file, please check if the change is
# also needed for src/distrib/utils/x_ifconfig.
# such stuff should be into Makefile.inc.

.include <bsd.own.mk>

RUMPPRG=	ifconfig
MAN=		ifconfig.8

SRCS=		af_atalk.c af_link.c carp.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/dist/pf/
SRCS+=		pfsync.c

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+=		af_inet6.c
.endif

.include "Makefile.common"

.if (${MKRUMP} != "no")
CPPFLAGS+=	-DRUMP_ACTION
LDADD.rump +=	-lrumpres
DPADD.rump +=	${LIBRUMPRES}
.endif

.include <bsd.prog.mk>
