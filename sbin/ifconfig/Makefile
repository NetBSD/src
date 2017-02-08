#	$NetBSD: Makefile,v 1.59 2017/02/08 23:22:43 rin Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

# when making a change to this file, please check if the change is
# also needed for src/distrib/utils/x_ifconfig.
# such stuff should be into Makefile.inc.

.include <bsd.own.mk>

RUMPPRG=ifconfig
MAN=	ifconfig.8

#DBG+=-g
SRCS= af_atalk.c af_link.c carp.c
.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+= af_inet6.c
.endif

.include "Makefile.common"

.PATH:		${.CURDIR}/../../lib/libc/net
RUMPSRCS= getifaddrs.c getnameinfo.c if_indextoname.c
.if (${MKRUMP} != "no")
CPPFLAGS+= -DRUMP_ACTION
.endif

CPPFLAGS+=-I${NETBSDSRCDIR}/sys/dist/pf/
SRCS+= pfsync.c

.include <bsd.prog.mk>
