#	$NetBSD: Makefile,v 1.2.4.1 2007/11/06 23:34:31 matt Exp $
#

.include <bsd.own.mk>

LIB=		ukfs

SRCS=		ukfs.c

RUMPKERNEL=	no
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${LIBRUMPDIR}

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.lib.mk>
