#	$NetBSD: Makefile,v 1.56.4.1 2017/04/21 16:53:13 bouyer Exp $
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

.PATH:		${.CURDIR}/../../lib/libc/net
RUMPSRCS=	getifaddrs.c getnameinfo.c if_indextoname.c
.if (${MKRUMP} != "no")
CPPFLAGS+=	-DRUMP_ACTION
.endif

.include <bsd.prog.mk>
