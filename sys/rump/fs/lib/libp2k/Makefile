#	$NetBSD: Makefile,v 1.3.2.3 2007/10/09 13:45:01 ad Exp $
#

.include <bsd.own.mk>

LIB=		p2k

SRCS=		p2k.c

RUMPKERNEL=	no
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${NETBSDSRCDIR}/sys/rump/fs/lib/libukfs

.include <bsd.lib.mk>
