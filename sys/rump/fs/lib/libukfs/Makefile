#	$NetBSD: Makefile,v 1.1.2.3 2007/09/03 10:23:53 skrll Exp $
#

.include <bsd.own.mk>

LIB=		ukfs

SRCS=		ukfs.c

RUMPKERNEL=	no
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/rump/librump/rumpkern		\
		-I${LIBRUMPDIR}

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.lib.mk>
