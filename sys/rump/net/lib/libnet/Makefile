#	$NetBSD: Makefile,v 1.13.6.1 2012/04/29 23:05:07 mrg Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../compat/common

LIB=	rumpnet_net

# iffy stuff
SRCS=	if.c if_loop.c route.c rtbl.c rtsock.c rtsock_50.c raw_usrreq.c	\
	raw_cb.c if_media.c link_proto.c net_stats.c if_ethersubr.c rfc6056.c
SRCS+=	if_43.c pfil.c uipc_syscalls_50.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/opt -I${.CURDIR}/../libnetinet/opt
CPPFLAGS+=	-DCOMPAT_OIFREQ -DCOMPAT_OIFDATA

.include "${.CURDIR}/../libnetinet/Makefile.inc"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
