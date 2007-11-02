#	$NetBSD: Makefile,v 1.4.8.2 2007/11/02 12:43:41 joerg Exp $
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
