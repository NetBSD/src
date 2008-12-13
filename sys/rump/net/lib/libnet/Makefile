#	$NetBSD: Makefile,v 1.2.2.3 2008/12/13 01:15:34 haad Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_net

# iffy stuff
SRCS=	if.c if_loop.c route.c rtsock.c raw_usrreq.c raw_cb.c		\
	if_media.c link_proto.c net_stats.c if_ethersubr.c

CPPFLAGS+=	-I${.CURDIR}/opt -I${.CURDIR}/../libnetinet/opt
CFLAGS+=	-fno-strict-aliasing   # void ** cast in route.c is killing me

.include "${.CURDIR}/../libnetinet/Makefile.inc"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
