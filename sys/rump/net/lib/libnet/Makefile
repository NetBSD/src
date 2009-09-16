#	$NetBSD: Makefile,v 1.4.4.4 2009/09/16 13:38:06 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_net

# iffy stuff
SRCS=	if.c if_loop.c route.c rtsock.c raw_usrreq.c raw_cb.c		\
	if_media.c link_proto.c net_stats.c if_ethersubr.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/opt -I${.CURDIR}/../libnetinet/opt

RUMP_ISDOMAIN=

.include "${.CURDIR}/../libnetinet/Makefile.inc"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
