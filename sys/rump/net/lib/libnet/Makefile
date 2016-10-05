#	$NetBSD: Makefile,v 1.20.6.5 2016/10/05 20:56:11 skrll Exp $
#


.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../compat/common

LIB=	rumpnet_net
COMMENT=Network interface and routing support

IOCONF=	NET.ioconf
# iffy stuff
SRCS=	if.c if_loop.c route.c rtsock.c raw_usrreq.c			\
	raw_cb.c if_media.c link_proto.c net_stats.c if_ethersubr.c	\
	if_spppsubr.c
SRCS+=	if_43.c pfil.c
SRCS+=	if_llatbl.c
SRCS+=	net_component.c

.include <bsd.init.mk>

.if !empty(RUMP_NBCOMPAT:M50)
SRCS+=	rtsock_50.c uipc_syscalls_50.c
.endif

.if !empty(RUMP_NBCOMPAT:M70)
SRCS+=	rtsock_70.c uipc_usrreq_70.c
.endif

.include "${.CURDIR}/../libnetinet/Makefile.inc"
.include "${.CURDIR}/../libnetinet6/Makefile.inc"
.include "${.CURDIR}/../libnetmpls/Makefile.inc"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
