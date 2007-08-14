#	$NetBSD: Makefile,v 1.2 2007/08/14 15:56:16 pooka Exp $
#

LIB=		p2k

SRCS=		p2k.c

RUMPKERNEL=	no
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${NETBSDSRCDIR}/sys/rump/fs/lib/libukfs

.include <bsd.lib.mk>
