#	$NetBSD: Makefile,v 1.22 2015/04/23 07:56:03 pooka Exp $
#

.include <bsd.init.mk>

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../compat/common

LIB=	rumpnet_net

# iffy stuff
SRCS=	if.c if_loop.c route.c rtsock.c raw_usrreq.c			\
	raw_cb.c if_media.c link_proto.c net_stats.c if_ethersubr.c
SRCS+=	if_43.c pfil.c
SRCS+=	net_component.c

.if !empty(RUMP_COMPAT:M50)
SRCS+=	rtsock_50.c uipc_syscalls_50.c
.endif

CPPFLAGS+=	-I${.CURDIR}/opt -I${.CURDIR}/../libnetinet/opt

.include "${.CURDIR}/../libnetinet/Makefile.inc"
.include "${.CURDIR}/../libnetinet6/Makefile.inc"
.include "${.CURDIR}/../libnetmpls/Makefile.inc"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
