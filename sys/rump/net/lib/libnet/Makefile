#	$NetBSD: Makefile,v 1.9 2010/12/07 20:39:07 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../compat/common

LIB=	rumpnet_net

# iffy stuff
SRCS=	if.c if_bridge.c if_loop.c route.c rtsock.c raw_usrreq.c	\
	raw_cb.c if_media.c link_proto.c net_stats.c if_ethersubr.c
SRCS+=	bridgestp.c
SRCS+=	if_43.c uipc_syscalls_50.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/opt -I${.CURDIR}/../libnetinet/opt
CPPFLAGS+=	-DCOMPAT_OIFREQ -DCOMPAT_OIFDATA

.include "${.CURDIR}/../libnetinet/Makefile.inc"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
