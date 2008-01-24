#	$NetBSD: Makefile,v 1.5 2008/01/24 22:41:08 pooka Exp $
#

.include <bsd.own.mk>

LIB=		rumpuser

SRCS=		rumpuser.c rumpuser_pth.c

#CPPFLAGS+=	-DRUMP_WITHOUT_THREADS

RUMPKERNEL=	no

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.lib.mk>
