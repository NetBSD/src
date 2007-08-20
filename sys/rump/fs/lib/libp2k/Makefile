#	$NetBSD: Makefile,v 1.3.2.2 2007/08/20 22:07:20 ad Exp $
#

LIB=		p2k

SRCS=		p2k.c

RUMPKERNEL=	no
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${NETBSDSRCDIR}/sys/rump/fs/lib/libukfs

.include <bsd.lib.mk>
