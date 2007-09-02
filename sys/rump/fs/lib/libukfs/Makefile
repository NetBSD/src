#	$NetBSD: Makefile,v 1.3 2007/09/02 12:00:12 pooka Exp $
#

.include <bsd.own.mk>

LIB=		ukfs

SRCS=		ukfs.c

RUMPKERNEL=	no
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${LIBRUMPDIR}

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.lib.mk>
