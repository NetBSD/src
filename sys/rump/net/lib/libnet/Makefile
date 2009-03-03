#	$NetBSD: Makefile,v 1.2.4.2 2009/03/03 18:34:30 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_net

# iffy stuff
SRCS=	if.c if_loop.c route.c rtsock.c raw_usrreq.c raw_cb.c		\
	if_media.c link_proto.c net_stats.c if_ethersubr.c

CPPFLAGS+=	-I${.CURDIR}/opt -I${.CURDIR}/../libnetinet/opt

.include "${.CURDIR}/../libnetinet/Makefile.inc"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
