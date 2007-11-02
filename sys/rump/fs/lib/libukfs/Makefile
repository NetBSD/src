#	$NetBSD: Makefile,v 1.3.8.2 2007/11/02 12:43:46 joerg Exp $
#

.include <bsd.own.mk>

LIB=		ukfs

SRCS=		ukfs.c

RUMPKERNEL=	no
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${LIBRUMPDIR}

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.lib.mk>
