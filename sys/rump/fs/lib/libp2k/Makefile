#	$NetBSD: Makefile,v 1.4 2007/09/02 12:00:12 pooka Exp $
#

.include <bsd.own.mk>

LIB=		p2k

SRCS=		p2k.c

RUMPKERNEL=	no
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${NETBSDSRCDIR}/sys/rump/fs/lib/libukfs		\
		-I${LIBRUMPDIR}

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.lib.mk>
