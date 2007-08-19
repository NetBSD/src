#	$NetBSD: Makefile,v 1.2 2007/08/19 21:24:21 pooka Exp $
#

LIB=		ukfs

SRCS=		ukfs.c

RUMPKERNEL=	no
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${LIBRUMPDIR}

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.lib.mk>
